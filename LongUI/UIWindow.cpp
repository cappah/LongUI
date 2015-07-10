﻿#include "LongUI.h"

// 位图规划:
/*
-----
|1|2|
-----
|3|4|
-----
1-2     标题相关
3       插入符
*/

#define MakeAsUnit(a) (((a) + (LongUITargetBitmapUnitSize-1)) / LongUITargetBitmapUnitSize * LongUITargetBitmapUnitSize)


// UIWindow 构造函数
LongUI::UIWindow::UIWindow(pugi::xml_node node,
    UIWindow* parent) noexcept : Super(node), m_uiRenderQueue(this) {
    assert(node && "<LongUI::UIWindow::UIWindow> window_node null");
    // flag 区
    {
        uint32_t flag = this->flags;
        // 检查DComposition标记
        if (node.attribute("dcomp").as_bool(true)) {
            flag |= Flag_Window_DComposition;
        }
        // 目前不支持非Flag_Window_DComposition
        // 可能会发生未知错误
        assert(flag & Flag_Window_DComposition && "NOT IMPL");
        // 检查FullRendering标记
        if (node.attribute("fullrender").as_bool(false)) {
            flag |= Flag_Window_FullRendering;
        }
        else {
            //this->reset_renderqueue();
        }
        // 检查RenderedOnParentWindow标记
        if (node.attribute("renderonparent").as_bool(false)) {
            flag |= Flag_Window_RenderedOnParentWindow;
        }
        // 检查alwaysrendering
        if (node.attribute("alwaysrendering").as_bool(false)) {
            flag |= Flag_Window_AlwaysRendering;
        }
        force_cast(this->flags) = static_cast<LongUIFlag>(flag);
    }
    // 窗口区
    {
        // 默认样式
        DWORD window_style = WS_OVERLAPPEDWINDOW;
        // 设置窗口大小
        RECT window_rect = { 0, 0, LongUIDefaultWindowWidth, LongUIDefaultWindowHeight };
        // 默认
        if (this->width == 0.f) {
            this->width = static_cast<float>(LongUIDefaultWindowWidth);
        }
        else {
            window_rect.right = static_cast<LONG>(this->width);
        }
        // 更新
        if (this->height == 0.f) {
            this->height = static_cast<float>(LongUIDefaultWindowHeight);
        }
        else {
            window_rect.bottom = static_cast<LONG>(this->height);
        }
        force_cast(this->windows_size.width) = window_rect.left;
        force_cast(this->windows_size.height) = window_rect.bottom;
        visible_rect.right = this->width;
        visible_rect.bottom = this->height;
        visible_size.width = this->width;
        visible_size.height = this->height;
        // 调整大小
        ::AdjustWindowRect(&window_rect, window_style, FALSE);
        // 居中
        window_rect.right -= window_rect.left;
        window_rect.bottom -= window_rect.top;
        window_rect.left = (::GetSystemMetrics(SM_CXFULLSCREEN) - window_rect.right) / 2;
        window_rect.top = (::GetSystemMetrics(SM_CYFULLSCREEN) - window_rect.bottom) / 2;
        // 创建窗口
        m_hwnd = ::CreateWindowExW(
            //WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
            (this->flags & Flag_Window_DComposition) ? WS_EX_NOREDIRECTIONBITMAP : 0,
            L"LongUIWindow", L"Nameless",
            WS_OVERLAPPEDWINDOW,
            window_rect.left, window_rect.top, window_rect.right, window_rect.bottom,
            parent ? parent->GetHwnd() : nullptr, nullptr,
            ::GetModuleHandleW(nullptr),
            this
            );
    }
    //SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);
    // 设置Hover
    m_csTME.cbSize = sizeof(m_csTME);
    m_csTME.dwFlags = TME_HOVER | TME_LEAVE;
    m_csTME.hwndTrack = m_hwnd;
    m_csTME.dwHoverTime = LongUIDefaultHoverTime;
    // 创建闪烁计时器
    m_idBlinkTimer = ::SetTimer(m_hwnd, 0, ::GetCaretBlinkTime(), nullptr);
    // 添加窗口
    UIManager.AddWindow(this);
    this->clear_color.a = 0.85f;
    // 创建帮助器
    ::CoCreateInstance(
        CLSID_DragDropHelper, 
        nullptr, 
        CLSCTX_INPROC_SERVER,
        LongUI_IID_PV_ARGS(m_pDropTargetHelper)
        );
    // 注册拖拽目标
    ::RegisterDragDrop(m_hwnd, this);
    // 显示窗口
    ::ShowWindow(m_hwnd, SW_SHOW);
    // 开始计时
    m_timer.Start();
    // 所在窗口就是自己
    m_pWindow = this;
    // 自己的父类就是自己以保证parent不为null
    force_cast(this->parent) = this;
    // 呈现参数
    ZeroMemory(&m_rcScroll, sizeof(m_dirtyRects));
    ZeroMemory(m_dirtyRects, sizeof(m_dirtyRects));
    m_present = { 0, m_dirtyRects, nullptr, nullptr };
}

// UIWindow 析构函数
LongUI::UIWindow::~UIWindow() noexcept {
    // 取消注册
    ::RevokeDragDrop(m_hwnd);
    // 摧毁窗口
    ::DestroyWindow(m_hwnd);
    // 移除窗口
    UIManager.RemoveWindow(this);
    // 杀掉!
    ::KillTimer(m_hwnd, m_idBlinkTimer);
    // 释放资源
    this->release_data();
    // 释放数据
    ::SafeRelease(m_pDropTargetHelper);
    ::SafeRelease(m_pCurDataObject);
}


// 注册
void LongUI::UIWindow::RegisterOffScreenRender(UIControl* c, bool is3d) noexcept {
    // 检查
#ifdef _DEBUG
    auto itr = std::find(m_vRegisteredControl.begin(), m_vRegisteredControl.end(), c);
    if (itr != m_vRegisteredControl.end()) {
        UIManager << DL_Warning << L"control: [" << c->GetNameStr() << L"] existed" << LongUI::endl;
        return;
    }
#endif
    try {
        if (is3d) {
            m_vRegisteredControl.insert(m_vRegisteredControl.begin(), c);
        }
        else {
            m_vRegisteredControl.push_back(c);
        }
    }
    catch (...) {
        UIManager << DL_Warning << L"insert failed" << LongUI::endl;
    }
}

// 反注册
void LongUI::UIWindow::UnRegisterOffScreenRender(UIControl* c) noexcept {
    auto itr = std::find(m_vRegisteredControl.begin(), m_vRegisteredControl.end(), c);
    if (itr != m_vRegisteredControl.end()) {
        m_vRegisteredControl.erase(itr);
    }
#ifdef _DEBUG
    else {
        UIManager << DL_Warning << L"control: [" << c->GetNameStr() << L"] not found" << LongUI::endl;
    }
#endif
}


// 设置插入符号
void LongUI::UIWindow::SetCaretPos(UIControl* c, float x, float y) noexcept {
    if (!m_cShowCaret) return;
    // 转换为像素坐标
    auto pt = D2D1::Point2F(x, y);
    if (c) {
        // FIXME
        // TODO: FIX IT
        pt = LongUI::TransformPoint(c->parent->world, pt);
    }
    m_bCaretIn = true;
    const register int intx = static_cast<int>(pt.x);
    const register int inty = static_cast<int>(pt.y);
    const register int oldx = static_cast<int>(m_rcCaretPx.left);
    const register int oldy = static_cast<int>(m_rcCaretPx.top);
    if (oldx != intx || oldy != inty) {
        this->refresh_caret();
        m_rcCaretPx.left = intx; m_rcCaretPx.top = inty;
        ::SetCaretPos(intx, inty);
#if 0
        if (!m_pd2dBitmapAE) return;
        m_pTargetBimtap->CopyFromBitmap(nullptr, m_pd2dBitmapAE, nullptr);
        this->draw_caret();
        /*const register int intw = static_cast<int>(m_rcCaret.width) + 1;
        const register int inth = static_cast<int>(m_rcCaret.height) + 1;
        RECT rects[] = {
            { oldx, oldy, oldx + intw,oldy + inth },
            { intx, inty, intx + intw,inty + inth },
        };*/
        /*::(L"rects: {%d, %d, %d, %d} {%d, %d, %d, %d}\n",
            rects[0].left, rects[0].top, rects[0].right, rects[0].bottom,
            rects[1].left, rects[1].top, rects[1].right, rects[1].bottom
            );*/
        DXGI_PRESENT_PARAMETERS para = { 0, nullptr, nullptr, nullptr };
        // 脏矩形刷新
        m_pSwapChain->Present1(0, 0, &para);
#endif
    }
}

// 创建插入符号
void LongUI::UIWindow::CreateCaret(float width, float height) noexcept {
    this->refresh_caret();
    // TODO: 转换为像素单位
    m_rcCaretPx.width = static_cast<decltype(m_rcCaretPx.height)>(width);
    m_rcCaretPx.height = static_cast<decltype(m_rcCaretPx.width)>(height);
    if (!m_rcCaretPx.width) m_rcCaretPx.width = 1;
    if (!m_rcCaretPx.height) m_rcCaretPx.height = 1;
}

// 显示插入符号
void LongUI::UIWindow::ShowCaret() noexcept {
    ++m_cShowCaret;
    // 创建AE位图
    //if (!m_pd2dBitmapAE) {
        //this->recreate_ae_bitmap();
    //}
}

// 异常插入符号
void LongUI::UIWindow::HideCaret() noexcept { 
    if (m_cShowCaret) {
        --m_cShowCaret;
    }
#ifdef _DEBUG
    else {
        UIManager << DL_Warning << L"m_cShowCaret alread to 0" << LongUI::endl;
    }
#endif
    if (!m_cShowCaret) m_bCaretIn = false;
}

// 查找控件
auto LongUI::UIWindow::FindControl(const CUIString& str) noexcept -> UIControl * {
    // 查找控件
    const auto itr = m_mapString2Control.find(str);
    // 未找到返回空
    if (itr == m_mapString2Control.cend()) {
        // 警告
        UIManager << DL_Warning << L"Control Not Found:\n  " << str << LongUI::endl;
        return nullptr;
    }
    // 找到就返回指针
    else {
        return reinterpret_cast<LongUI::UIControl*>(itr->second);
    }
}

// 添加控件
void LongUI::UIWindow::AddControl(const std::pair<CUIString, void*>& pair) noexcept {
    // 有效
    if (pair.first != L"") {
        try {
            m_mapString2Control.insert(pair);
        }
        catch (...) {
            ShowErrorWithStr(L"Failed to add control");
        }
    }
}

// release data
void LongUI::UIWindow::release_data() noexcept {
    if (m_hVSync) {
        ::CloseHandle(m_hVSync);
        m_hVSync = nullptr;
    }
    // 释放资源
    ::SafeRelease(m_pTargetBimtap);
    ::SafeRelease(m_pSwapChain);
    ::SafeRelease(m_pDcompDevice);
    ::SafeRelease(m_pDcompTarget);
    ::SafeRelease(m_pDcompVisual);
}

// 刻画插入符号
void LongUI::UIWindow::draw_caret() noexcept {
    /*// 不能在BeginDraw/EndDraw之间调用
    D2D1_POINT_2U pt = { m_rcCaretPx.left, m_rcCaretPx.top };
    D2D1_RECT_U src_rect;
    src_rect.top = LongUIWindowPlanningBitmap / 2;
    src_rect.left = m_bCaretIn ? 0 : LongUIWindowPlanningBitmap / 4;
    src_rect.right = src_rect.left + m_rcCaretPx.width;
    src_rect.bottom = src_rect.top + m_rcCaretPx.height;
    m_pTargetBimtap->CopyFromBitmap(
        &pt, m_pBitmapPlanning, &src_rect
        );*/
}

// 更新插入符号
void LongUI::UIWindow::refresh_caret() noexcept {
    // 不能在BeginDraw/EndDraw之间调用
    // TODO: 完成位图复制
}

// 设置呈现
void LongUI::UIWindow::set_present() noexcept {
    // 显示光标?
    if (m_cShowCaret) {
        this->draw_caret();
        m_dirtyRects[m_present.DirtyRectsCount].left 
            = std::max(static_cast<LONG>(m_rcCaretPx.left), long(0));
        m_dirtyRects[m_present.DirtyRectsCount].top 
            = std::max(static_cast<LONG>(m_rcCaretPx.top), long(0));
        m_dirtyRects[m_present.DirtyRectsCount].right 
            = std::min(static_cast<LONG>(m_rcCaretPx.left + m_rcCaretPx.width), long(this->windows_size.width));
        m_dirtyRects[m_present.DirtyRectsCount].bottom 
            = std::min(static_cast<LONG>(m_rcCaretPx.top + m_rcCaretPx.height), long(this->windows_size.height));
        ++m_present.DirtyRectsCount;
    }
    // 存在脏矩形?
    if (m_present.DirtyRectsCount) {
        m_present.pScrollRect = &m_rcScroll;
#ifdef _DEBUG
        static RECT s_rects[LongUIDirtyControlSize + 2];
        ::memcpy(s_rects, m_dirtyRects, m_present.DirtyRectsCount * sizeof(RECT));
        m_present.pDirtyRects = s_rects;
        s_rects[m_present.DirtyRectsCount] = { 0, 0, 128, 35 };
        ++m_present.DirtyRectsCount;
#endif
    }
    else {
        m_present.pScrollRect = nullptr;
    }
}

// begin draw
void LongUI::UIWindow::BeginDraw() const noexcept {
    // 离屏渲染
    if (!m_vRegisteredControl.empty()) {
        for (auto i : m_vRegisteredControl) {
            auto ctrl = reinterpret_cast<UIControl*>(i);
            assert(ctrl->parent && "check it");
            m_pRenderTarget->SetTransform(&ctrl->parent->world);
            ctrl->Render(RenderType::Type_RenderOffScreen);
        }
    }
    // 设为当前渲染对象
    m_pRenderTarget->SetTarget(m_pTargetBimtap);
    // 开始渲染
    m_pRenderTarget->BeginDraw();
    // 设置转换矩阵
    m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    // 清空背景
    m_pRenderTarget->Clear(this->clear_color);
}

// 结束渲染
void LongUI::UIWindow::EndDraw() const noexcept {
    // 结束渲染
    m_pRenderTarget->EndDraw();
    // !!!!!!!一切小心!
    UIWindow* pThis = const_cast<UIWindow*>(this);
    pThis->set_present();
    HRESULT hr = m_pSwapChain->Present1(1, 0, &m_present);
    // 收到重建消息时 重建UI
#ifdef _DEBUG
    if (hr == DXGI_ERROR_DEVICE_REMOVED 
        || hr == DXGI_ERROR_DEVICE_RESET 
        || test_D2DERR_RECREATE_TARGET) {
        pThis->test_D2DERR_RECREATE_TARGET = false;
        UIManager << DL_Hint << L"D2DERR_RECREATE_TARGET!" << LongUI::endl;
        hr = UIManager.RecreateResources();
        if (FAILED(hr)) {
            UIManager << DL_Hint << L"But, Recreate Failed!!!" << LongUI::endl;
            UIManager << DL_Error << L"Recreate Failed!!!" << LongUI::endl;
        }
    }
#else
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        hr = UIManager.RecreateResources();
    }
#endif
    // 检查
    AssertHR(hr);
}


// UI窗口: 刷新
void LongUI::UIWindow::Update() noexcept {
    // 新窗口大小?
    if (m_bNewSize) {
        this->OnResize();
        m_bNewSize = false;
    }
    // 设置间隔时间
    m_fDeltaTime = m_timer.Delta_s<decltype(m_fDeltaTime)>();
    m_timer.MovStartEnd();
    auto current_unit = m_uiRenderQueue.GetCurrentUnit();
    // 没有就不刷新了
    m_bRendered = !!current_unit->length;
    if (!current_unit->length) return;
    // 全刷新?
    if (current_unit->units[0] == static_cast<UIControl*>(this)) {
        m_present.DirtyRectsCount = 0;
        // 交给父类处理
        Super::Update();
    }
    // 部分刷新
    else {
        m_present.DirtyRectsCount = current_unit->length;
        // 更新脏矩形
        for (uint32_t i = 0ui32; i < current_unit->length; ++i) {
            auto ctrl = current_unit->units[i];
            assert(ctrl->parent && "check it");
            // 设置转换矩阵
            ctrl->Update();
            // 限制转换
            m_dirtyRects[i].left = static_cast<LONG>(ctrl->visible_rect.left);
            m_dirtyRects[i].top = static_cast<LONG>(ctrl->visible_rect.top);
            m_dirtyRects[i].right = static_cast<LONG>(std::ceil(ctrl->visible_rect.right));
            m_dirtyRects[i].bottom = static_cast<LONG>(std::ceil(ctrl->visible_rect.bottom));
        }
    }
    // 调试
#ifdef _DEBUG
    if (!m_present.DirtyRectsCount) {
        ++full_render_counter;
    }
    else {
        ++dirty_render_counter;
    }
#endif
}

// UIWindow 渲染 
void LongUI::UIWindow::Render(RenderType type)const noexcept  {
    if (type != RenderType::Type_Render) return ;
    const auto current_unit = m_uiRenderQueue.GetCurrentUnit();
    // 全刷新: 继承父类
    if (!m_present.DirtyRectsCount) {
        Super::Render(RenderType::Type_Render);
    }
    // 部分刷新:
    else {
#if 1
        // 先排序
        UIControl* units[LongUIDirtyControlSize];
        assert(current_unit->length < LongUIDirtyControlSize);
        auto length_for_units = current_unit->length;
        ::memcpy(units, current_unit->units, length_for_units * sizeof(void*));
        std::sort(units, units + length_for_units, [](UIControl* a, UIControl* b) noexcept {
            return a->priority > b->priority;
        });
        if (current_unit->length >= 2) {
            assert(units[0]->priority >= units[1]->priority);
        }
        // 再渲染
        for (auto unit = units; unit < units + length_for_units; ++unit) {
            auto ctrl = *unit;
            // 设置转换矩阵
            D2D1_MATRIX_3X2_F matrix; ctrl->GetWorldTransform(matrix);
            m_pRenderTarget->SetTransform(&matrix);
            ctrl->Render(RenderType::Type_Render);
    }
#else
        // 再渲染
        for (uint32_t i = 0ui32; i < current_unit->length; ++i) {
            auto ctrl = current_unit->units[i];
            // 设置转换矩阵
            D2D1_MATRIX_3X2_F matrix; ctrl->GetWorldTransform(matrix);
            m_pRenderTarget->SetTransform(&matrix);
            ctrl->Render(RenderType::Type_Render);
        }
#endif
    }
#ifdef _DEBUG
    // 调试输出
    {
        D2D1_MATRIX_3X2_F nowMatrix, iMatrix = D2D1::Matrix3x2F::Scale(0.45f, 0.45f);
        m_pRenderTarget->GetTransform(&nowMatrix);
        m_pRenderTarget->SetTransform(&iMatrix);
        wchar_t buffer[1024];
        auto length = ::swprintf(
            buffer, 1024,
            L"Full Rendering Count: %d\nDirty Rendering Count: %d\nThis DirtyRectsCount:%d",
            full_render_counter,
            dirty_render_counter,
            m_present.DirtyRectsCount
            );
        auto tf = UIManager.GetTextFormat(LongUIDefaultTextFormatIndex);
        auto ta = tf->GetTextAlignment();
        m_pBrush_SetBeforeUse->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_pRenderTarget->DrawText(
            buffer, length, tf,
            D2D1::RectF(0.f, 0.f, 1000.f, 70.f),
            m_pBrush_SetBeforeUse
            );
        tf->SetTextAlignment(ta);
        ::SafeRelease(tf);
        m_pRenderTarget->SetTransform(&nowMatrix);
    }
#endif
}

// UIWindow 事件处理
bool LongUI::UIWindow::DoEvent(const LongUI::EventArgument& _arg) noexcept {
    // 自己一般不处理LongUI事件
    if (_arg.sender) return Super::DoEvent(_arg);
    // 其他LongUI事件
    bool handled = false; UIControl* control_got = nullptr;
    // 处理事件
    switch (_arg.msg)
    {
        LongUI::EventArgument new_arg;
    case WM_SETCURSOR:
        // 设置光标
        ::SetCursor(now_cursor);
        break;
    /*case WM_DWMCOLORIZATIONCOLORCHANGED:
    {
        DWORD buffer = sizeof(color_a);
        ::RegGetValueA(
            HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\DWM",
            "ColorizationColor",
            RRF_RT_DWORD,
            nullptr,
            &color_a,
            &buffer
            );
        this->Invalidate(this);
    }
    break;*/
    case WM_MOUSEMOVE:
        handled = this->OnMouseMove(_arg);
        break;
    case WM_TIMER:
        // 闪烁?
        if (_arg.wParam_sys == 0 && m_cShowCaret) {
            // 闪烁
           /* m_present.DirtyRectsCount = 0;
            this->set_present();
            m_pSwapChain->Present1(0, 0, &m_present);*/
            m_bCaretIn = !m_bCaretIn;
        }
        handled = true;
        break;
    case WM_LBUTTONDOWN:    // 按下鼠标左键
        // 查找子控件
        control_got = this->FindControl(_arg.pt);
        // 控件有效
        if (control_got && control_got != m_pFocusedControl){
            new_arg = _arg;
            new_arg.sender = this;
            if (m_pFocusedControl){
                new_arg.event = LongUI::Event::Event_KillFocus;
                m_pFocusedControl->DoEvent(new_arg);
            }
            new_arg.event = LongUI::Event::Event_SetFocus;
            // 控件响应了?
            m_pFocusedControl = control_got->DoEvent(new_arg) ? control_got : nullptr;
        }
        break;
    /*case WM_NCHITTEST:
        _arg.lr = HTCAPTION;
        handled = true;
        break;*/
    case WM_SETFOCUS:
        ::CreateCaret(m_hwnd, nullptr, 1, 1);
        handled = true;
        break;
    case WM_KILLFOCUS:
        // 存在焦点控件
        if (m_pFocusedControl){
            new_arg = _arg;
            new_arg.sender = this;
            new_arg.event = LongUI::Event::Event_KillFocus;
            m_pFocusedControl->DoEvent(new_arg);
            m_pFocusedControl = nullptr;
        }
        ::DestroyCaret();
        handled = true;
        break;
    case WM_MOUSELEAVE:     // 鼠标移出窗口
        if (m_pPointedControl){
            new_arg = _arg;
            new_arg.sender = this;
            new_arg.event = LongUI::Event::Event_MouseLeave;
            m_pPointedControl->DoEvent(new_arg);
            m_pPointedControl = nullptr;
        }
        handled = true;
        break;
    case WM_SIZE:           // 改变大小
    {
        RECT rect; ::GetClientRect(m_hwnd, &rect);
        force_cast(this->windows_size.width) = rect.right - rect.left;
        force_cast(this->windows_size.height) = rect.bottom - rect.top;
        m_bNewSize = true;
    }
        handled = true;
        break;
    case WM_GETMINMAXINFO:  // 获取限制大小
        reinterpret_cast<MINMAXINFO*>(_arg.lParam_sys)->ptMinTrackSize.x = 128;
        reinterpret_cast<MINMAXINFO*>(_arg.lParam_sys)->ptMinTrackSize.y = 128;
        break;
    case WM_DISPLAYCHANGE:
        // 更新分辨率
        if (!(this->flags & Flag_Window_FullRendering)) {

        }
        // TODO: OOM处理
        break;
    case WM_CLOSE:          // 关闭窗口
        // 窗口关闭
        this->Close();
        handled = true;
        break;
    }
    // 处理
    if (handled) return true;
    // 处理控件
    register UIControl* processor = nullptr;
    // 鼠标事件交由捕获控件(优先)或者鼠标指向控件处理
    if (_arg.msg >= WM_MOUSEFIRST && _arg.msg <= WM_MOUSELAST) {
        processor = m_pCapturedControl ? m_pCapturedControl : m_pPointedControl;
    }
    // 其他事件交由焦点控件处理
    else {
        processor = m_pFocusedControl;
    }
    // 有就处理
    // 处理了就直接返回
    if (processor && processor->DoEvent(_arg)) {
        return true;
    }
    // 还是没有处理就交给父类处理
    return Super::DoEvent(_arg);
}

LongUINoinline auto PresentTest(IDXGISwapChain2* chain, DXGI_PRESENT_PARAMETERS* param) noexcept {
    return chain->Present1(1, 0, param);
}

// 等待重置同步
void LongUI::UIWindow::WaitVS() const noexcept {
    // 没渲染则强行渲染
#ifdef _DEBUG
    static bool first_time = true;
    if (first_time && !m_bRendered) {
        assert(!"should be rendered @ first time !");
    }
    first_time = false;
    // 渲染
    if (m_bRendered) {
        // 等待VS
        ::WaitForSingleObject(m_hVSync, INFINITE);
    }
    else {
        assert(!"error!");
    }
#else
    ::WaitForSingleObject(m_hVSync, INFINITE);
#endif

}

// 重置窗口大小
void LongUI::UIWindow::OnResize(bool force) noexcept {
    // 修改大小, 需要取消目标
    this->DrawSizeChanged(); m_pRenderTarget->SetTarget(nullptr);
    // 滚动
    m_rcScroll.right = this->windows_size.width;
    m_rcScroll.bottom = this->windows_size.height;

    visible_rect.right = static_cast<float>(this->windows_size.width);
    visible_rect.bottom = static_cast<float>(this->windows_size.height);
    visible_size.width = visible_rect.right;
    visible_size.height = visible_rect.bottom;
    //

    auto rect_right = MakeAsUnit(this->windows_size.width);
    auto rect_bottom = MakeAsUnit(this->windows_size.height);
    auto old_size = m_pTargetBimtap->GetPixelSize();
    register HRESULT hr = S_OK;
    // 强行 或者 小于才Resize
    if (force || old_size.width < uint32_t(rect_right) || old_size.height < uint32_t(rect_bottom)) {
        UIManager << DL_Hint << L"Window: [" << this->GetNameStr() << L"] \n\t\tTarget Bitmap Resize to " 
            << long(rect_right) << ", " << long(rect_bottom) << LongUI::endl;
        IDXGISurface* pDxgiBackBuffer = nullptr;
        ::SafeRelease(m_pTargetBimtap);
        hr = m_pSwapChain->ResizeBuffers(
            2, rect_right, rect_bottom, DXGI_FORMAT_B8G8R8A8_UNORM, 
            DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
            );
        // 检查
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            UIManager.RecreateResources();
            UIManager << DL_Hint << L"Recreate device" << LongUI::endl;
        }
        // 利用交换链获取Dxgi表面
        if (SUCCEEDED(hr)) {
            hr = m_pSwapChain->GetBuffer(0, LongUI_IID_PV_ARGS(pDxgiBackBuffer));
        }
        // 利用Dxgi表面创建位图
        if (SUCCEEDED(hr)) {
            D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.0f,
                96.0f
                );
            hr = m_pRenderTarget->CreateBitmapFromDxgiSurface(
                pDxgiBackBuffer,
                &bitmapProperties,
                &m_pTargetBimtap
                );
        }
        // 重建失败?
        if (FAILED(hr)) {
            UIManager << DL_Error << L" Recreate FAILED!" << LongUI::endl;
            AssertHR(hr);
        }
        ::SafeRelease(pDxgiBackBuffer);
    }
    // 强行刷新一帧
    this->Invalidate(this);
}


// UIWindow 重建
auto LongUI::UIWindow::Recreate(LongUIRenderTarget* newRT) noexcept ->HRESULT {
    // UIWindow::Recreate参数不会为nullptr
    assert(newRT && "bad argument");
    // DXGI Surface 后台缓冲
    IDXGISurface*                       pDxgiBackBuffer = nullptr;
    IDXGISwapChain1*                    pSwapChain = nullptr;
    this->release_data();
    // 创建交换链
    IDXGIFactory2* pDxgiFactory = UIManager;
    assert(pDxgiFactory);
    HRESULT hr = S_OK;
    // 创建交换链
    if (SUCCEEDED(hr)) {
        RECT rect = { 0 }; ::GetClientRect(m_hwnd, &rect);
        // 交换链信息
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = MakeAsUnit(rect.right - rect.left);
        swapChainDesc.Height = MakeAsUnit(rect.bottom - rect.top);
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        // 滚动
        m_rcScroll.right = rect.right - rect.left;
        m_rcScroll.bottom = rect.bottom - rect.top;
        if (this->flags & Flag_Window_DComposition) {
            // DirectComposition桌面应用程序
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            // 创建DirectComposition交换链
            hr = pDxgiFactory->CreateSwapChainForComposition(
                UIManager_DXGIDevice,
                &swapChainDesc,
                nullptr,
                &pSwapChain
                );
        }
        else {
            // 一般桌面应用程序
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            // 利用窗口句柄创建交换链
            hr = pDxgiFactory->CreateSwapChainForHwnd(
                UIManager_D3DDevice,
                m_hwnd,
                &swapChainDesc,
                nullptr,
                nullptr,
                &pSwapChain
                );
        }
    }
    // 获取交换链V2
    if (SUCCEEDED(hr)) {
        hr = pSwapChain->QueryInterface(
            IID_IDXGISwapChain2,
            reinterpret_cast<void**>(&m_pSwapChain)
            );
    }
    // 获取垂直等待事件
    if (SUCCEEDED(hr)) {
        m_hVSync = m_pSwapChain->GetFrameLatencyWaitableObject();
    }
    // 确保DXGI队列里边不会超过一帧
    if (SUCCEEDED(hr)) {
        hr = UIManager_DXGIDevice->SetMaximumFrameLatency(1);
    }
    // 利用交换链获取Dxgi表面
    if (SUCCEEDED(hr)) {
        hr = m_pSwapChain->GetBuffer(0, LongUI_IID_PV_ARGS(pDxgiBackBuffer));
    }
    // 利用Dxgi表面创建位图
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f
            );
        hr = newRT->CreateBitmapFromDxgiSurface(
            pDxgiBackBuffer,
            &bitmapProperties,
            &m_pTargetBimtap
            );
    }
    // 使用DComp
    if (this->flags & Flag_Window_DComposition) {
        // 创建直接组合(Direct Composition)设备
        if (SUCCEEDED(hr)) {
            hr = LongUI::Dll::DCompositionCreateDevice(
                UIManager_DXGIDevice,
                LongUI_IID_PV_ARGS(m_pDcompDevice)
                );
        }
        // 创建直接组合(Direct Composition)目标
        if (SUCCEEDED(hr)) {
            hr = m_pDcompDevice->CreateTargetForHwnd(
                m_hwnd, true, &m_pDcompTarget
                );
        }
        // 创建直接组合(Direct Composition)视觉
        if (SUCCEEDED(hr)) {
            hr = m_pDcompDevice->CreateVisual(&m_pDcompVisual);
        }
        // 设置当前交换链为视觉内容
        if (SUCCEEDED(hr)) {
            hr = m_pDcompVisual->SetContent(m_pSwapChain);
        }
        // 设置当前视觉为窗口目标
        if (SUCCEEDED(hr)) {
            hr = m_pDcompTarget->SetRoot(m_pDcompVisual);
        }
        // 向系统提交
        if (SUCCEEDED(hr)) {
            hr = m_pDcompDevice->Commit();
        }
    }
    // 错误
    if (FAILED(hr)){
        UIManager << L"Recreate Failed!" << LongUI::endl;
        AssertHR(hr);
    }
    ::SafeRelease(pDxgiBackBuffer);
    ::SafeRelease(pSwapChain);
    {
        // 获取屏幕刷新率
        DEVMODEW mode = { 0 };
        ::EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode);
        m_uiRenderQueue.Reset(mode.dmDisplayFrequency);
        // 强行刷新一帧
        this->Invalidate(this);
    }
    // 重建 子控件UI
    return Super::Recreate(newRT);
}

// UIWindow 关闭控件
void LongUI::UIWindow::Close() noexcept {
    // 删除对象
    delete this;
    // 退出
    UIManager.Exit();
}


// 鼠标移动时候
bool LongUI::UIWindow::OnMouseMove(const LongUI::EventArgument& arg) noexcept {
    bool handled = false;
    do {
        ::TrackMouseEvent(&m_csTME);
        if (m_normalLParam != arg.lParam_sys) {
            m_normalLParam = arg.lParam_sys;
        }
        else {
            handled = true;
            break;
        }
        // 有待捕获控件
        if (m_pCapturedControl) {
            m_pCapturedControl->DoEvent(arg);
            handled = true;
            break;
        }
        // 查找子控件
        auto control_got = this->FindControl(arg.pt);
        if (control_got) {
            //UIManager << DL_Hint << "FIND: " << control_got << endl;
        }
        // 不同
        if (control_got != m_pPointedControl) {
            auto new_arg = arg;
            new_arg.sender = this;
            // 有效
            if (m_pPointedControl) {
                new_arg.event = LongUI::Event::Event_MouseLeave;
                m_pPointedControl->DoEvent(new_arg);
            }
            // 有效
            if ((m_pPointedControl = control_got)) {
                new_arg.event = LongUI::Event::Event_MouseEnter;
                m_pPointedControl->DoEvent(new_arg);
            }
        }
        // 相同
        else if (control_got) {
            control_got->DoEvent(arg);
        }
        handled = true;
    } while (false);
    return handled;
}

// ----------------- IDropTarget!!!! Yooooooooooo~-----

// 设置参数
void __fastcall SetLongUIEventArgument(LongUI::EventArgument& arg, HWND hwnd, POINTL pt) {
    // 获取窗口位置
    RECT rc = { 0 }; ::GetWindowRect(hwnd, &rc);
    // 映射到窗口坐标
    POINT ppt = { pt.x, pt.y };  ::ScreenToClient(hwnd, &ppt);
    // 查找对应控件
    arg = { 0 };
    arg.pt.x = static_cast<float>(ppt.x);
    arg.pt.y = static_cast<float>(ppt.y);

}

// 获取拖放效果
DWORD __fastcall GetDropEffect(DWORD grfKeyState, DWORD dwAllowed) {
    register DWORD dwEffect = 0;
    // 1. 检查pt来看是否允许drop操作在某个位置
    // 2. 计算出基于grfKeyState的drop效果
    if (grfKeyState & MK_CONTROL) {
        dwEffect = dwAllowed & DROPEFFECT_COPY;
    }
    else if (grfKeyState & MK_SHIFT) {
        dwEffect = dwAllowed & DROPEFFECT_MOVE;
    }
    // 3. 非键盘修饰符指定(或drop效果不允许), 因此基于drop源的效果
    if (dwEffect == 0) {
        if (dwAllowed & DROPEFFECT_COPY) dwEffect = DROPEFFECT_COPY;
        if (dwAllowed & DROPEFFECT_MOVE) dwEffect = DROPEFFECT_MOVE;
    }
    return dwEffect;
}

// IDropTarget::DragEnter 实现
HRESULT STDMETHODCALLTYPE LongUI::UIWindow::DragEnter(IDataObject* pDataObj,
    DWORD grfKeyState, POINTL pt, DWORD * pdwEffect) noexcept {
    m_bInDraging = true;
    // 检查参数
    if (!pDataObj) return E_INVALIDARG;
    // 取消聚焦窗口
    if(m_pFocusedControl){
        LongUI::EventArgument arg = { 0 };
        arg.sender = this;
        arg.event = LongUI::Event::Event_KillFocus;
        m_pFocusedControl->DoEvent(arg);
        m_pFocusedControl = nullptr;
    }
    // 保留数据
    ::SafeRelease(m_pCurDataObject);
    m_pCurDataObject = SafeAcquire(pDataObj);
    // 由帮助器处理
    POINT ppt = { pt.x, pt.y };
    if (m_pDropTargetHelper) {
        m_pDropTargetHelper->DragEnter(m_hwnd, pDataObj, &ppt, *pdwEffect);
    }
    return S_OK;
}


// IDropTarget::DragOver 实现
HRESULT STDMETHODCALLTYPE LongUI::UIWindow::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) noexcept {
    D2D1_POINT_2F pt2f = { static_cast<float>(pt.x), static_cast<float>(pt.y) };
    UIControl* control = nullptr;
    // 检查控件支持
    if ((control = this->FindControl(pt2f))) {
        LongUI::EventArgument arg;
        ::SetLongUIEventArgument(arg, m_hwnd, pt);
        arg.sender = this;
        // 第一个控件?
        if (m_pDragDropControl == control) {
            // 一样就是Over
            arg.event = LongUI::Event::Event_DragOver;
        }
        else {
            // 对老控件发送离开事件
            if (m_pDragDropControl) {
                arg.event = LongUI::Event::Event_DragLeave;
                m_pDragDropControl->DoEvent(arg);
            }
            // 新控件发送进入
            arg.event = LongUI::Event::Event_DragEnter;
            m_pDragDropControl = arg.ctrl;
        }
        arg.dataobj_cf = m_pCurDataObject;
        arg.outeffect_cf = pdwEffect;
        if (!arg.ctrl->DoEvent(arg)) *pdwEffect = DROPEFFECT_NONE;
    }
    else {
        // 不支持
        *pdwEffect = DROPEFFECT_NONE;
    }
    // 由帮助器处理
    if (m_pDropTargetHelper) {
        POINT ppt = { pt.x, pt.y };
        m_pDropTargetHelper->DragOver(&ppt, *pdwEffect);
    }
    return S_OK;
}

// IDropTarget::DragLeave 实现
HRESULT LongUI::UIWindow::DragLeave(void) noexcept {
    // 发送事件
    if (m_pDragDropControl) {
        LongUI::EventArgument arg = { 0 };
        arg.sender = this;
        arg.event = LongUI::Event::Event_DragLeave;
        m_pDragDropControl->DoEvent(arg);
        m_pDragDropControl = nullptr;
        // 存在捕获控件?
        /*if (m_pCapturedControl) {
            this->ReleaseCapture();
            /*arg.sender = nullptr;
            arg.msg = WM_LBUTTONUP;
            m_pCapturedControl->DoEvent(arg);
        }*/
    }
    /*OnDragLeave(m_hTargetWnd);*/
    m_pDragDropControl = nullptr;
    //m_isDataAvailable = TRUE;
    if (m_pDropTargetHelper) {
        m_pDropTargetHelper->DragLeave();
    }
    m_bInDraging = false;
    return S_OK;
}

// IDropTarget::Drop 实现
HRESULT LongUI::UIWindow::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) noexcept {
    // 发送事件
    if (m_pDragDropControl) {
        LongUI::EventArgument arg;
        ::SetLongUIEventArgument(arg, m_hwnd, pt);
        arg.sender = this;
        arg.event = LongUI::Event::Event_Drop;
        arg.dataobj_cf = m_pCurDataObject;
        arg.outeffect_cf = pdwEffect;
        // 发送事件
        m_pDragDropControl->DoEvent(arg);
        m_pDragDropControl = nullptr;
        
    }
    // 检查参数
    if (!pDataObj) return E_INVALIDARG;
    if (m_pDropTargetHelper){
        POINT ppt = { pt.x, pt.y };
        m_pDropTargetHelper->Drop(pDataObj, &ppt, *pdwEffect);
    }
    *pdwEffect = ::GetDropEffect(grfKeyState, *pdwEffect);
    return S_OK;
}
