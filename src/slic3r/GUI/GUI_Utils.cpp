#include "GUI_Utils.hpp"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <wx/toplevel.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/dcclient.h>

#include "libslic3r/Config.hpp"


namespace Slic3r {
namespace GUI {


wxTopLevelWindow* find_toplevel_parent(wxWindow *window)
{
    for (; window != nullptr; window = window->GetParent()) {
        if (window->IsTopLevel()) {
            return dynamic_cast<wxTopLevelWindow*>(window);
        }
    }

    return nullptr;
}

void on_window_geometry(wxTopLevelWindow *tlw, std::function<void()> callback)
{
#ifdef _WIN32
    // On windows, the wxEVT_SHOW is not received if the window is created maximized
    // cf. https://groups.google.com/forum/#!topic/wx-users/c7ntMt6piRI
    // OTOH the geometry is available very soon, so we can call the callback right away
    callback();
#elif defined __linux__
    tlw->Bind(wxEVT_SHOW, [=](wxShowEvent &evt) {
        // On Linux, the geometry is only available after wxEVT_SHOW + CallAfter
        // cf. https://groups.google.com/forum/?pli=1#!topic/wx-users/fERSXdpVwAI
        tlw->CallAfter([=]() { callback(); });
        evt.Skip();
    });
#elif defined __APPLE__
    tlw->Bind(wxEVT_SHOW, [=](wxShowEvent &evt) {
        callback();
        evt.Skip();
    });
#endif
}

enum { DPI_DEFAULT = 96 };

#ifdef _WIN32
template<const wchar_t *DLL, class F> F winapi_get_function(const char* fn_name) {
    static HINSTANCE dll = LoadLibraryExW(DLL, nullptr, 0);

    if (dll == nullptr) { return nullptr; }
    return (F)GetProcAddress(dll, fn_name);
}
#endif

int get_dpi_for_window(wxWindow *window)
{
#ifdef _WIN32
    typedef HRESULT (WINAPI *GetDpiForWindow_t)(HWND hwnd);
    typedef HRESULT (WINAPI *GetDpiForMonitor_t)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);

    static GetDpiForWindow_t GetDpiForWindow_fn = winapi_get_function<"User32.dll">("GetDpiForWindow");
    static GetDpiForMonitor_t GetDpiForMonitor_fn = winapi_get_function<"User32.dll">("GetDpiForMonitor");

    const HWND hwnd = window->GetHandle();

    if (GetDpiForWindow_fn != nullptr) {
        // We're on Windows 10, we have per-screen DPI settings
        return GetDpiForWindow_fn(hwnd);
    } else if (GetDpiForMonitor_fn != nullptr) {
        // We're on Windows 8.1, we have per-system DPI
        // Note: MonitorFromWindow() is available on all Windows,
        // but we inline MONITOR_DPI_TYPE enum here to not depend on Win8.1 headers.

        enum MONITOR_DPI_TYPE {
            MDT_EFFECTIVE_DPI = 0,
            MDT_ANGULAR_DPI = 1,
            MDT_RAW_DPI = 2,
            MDT_DEFAULT = MDT_EFFECTIVE_DPI,
        };

        const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        UINT dpiX;
        UINT dpiY;
        return GetDpiForMonitor_fn(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY) == S_OK ? dpiX : DPI_DEFAULT;
    } else {
        // We're on Windows earlier than 8.1, use DC

        const HDC dc = GetDC(hwnd);
        if (dc == NULL) { return DPI_DEFAULT; }
        return GetDeviceCaps(hdc, LOGPIXELSX);
    }
#elif defined __linux__
    // TODO
    return DPI_DEFAULT;
#elif defined __APPLE__
    // TODO
    return DPI_DEFAULT;
#endif
}



DPIDialog::DPIDialog(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
    : wxDialog(parent, id, title, pos, size, style, name)
{
    m_scale_factor = (float)get_dpi_for_window(this) / (float)DPI_DEFAULT;

    wxClientDC dc(this);
    const auto metrics = dc.GetFontMetrics();
    m_font_size = metrics.height;
    m_em_unit = metrics.averageWidth;
}

DPIDialog::~DPIDialog() {}

void DPIDialog::on_dpi_changed() {}

void DPIDialog::recalc_font()
{
    //
}


CheckboxFileDialog::ExtraPanel::ExtraPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    // WARN: wxMSW does some extra shenanigans to calc the extra control size.
    // It first calls the create function with a dummy empty wxDialog parent and saves its size.
    // Afterwards, the create function is called again with the real parent.
    // Additionally there's no way to pass any extra data to the create function (no closure),
    // which is why we have to this stuff here. Grrr!
    auto *dlg = dynamic_cast<CheckboxFileDialog*>(parent);
    const wxString checkbox_label(dlg != nullptr ? dlg->checkbox_label : wxString("String long enough to contain dlg->checkbox_label"));

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    cbox = new wxCheckBox(this, wxID_ANY, checkbox_label);
    cbox->SetValue(true);
    sizer->AddSpacer(5);
    sizer->Add(this->cbox, 0, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
    sizer->SetSizeHints(this);
}

wxWindow* CheckboxFileDialog::ExtraPanel::ctor(wxWindow *parent) {
    return new ExtraPanel(parent);
}

CheckboxFileDialog::CheckboxFileDialog(wxWindow *parent,
    const wxString &checkbox_label,
    bool checkbox_value,
    const wxString &message,
    const wxString &default_dir,
    const wxString &default_file,
    const wxString &wildcard,
    long style,
    const wxPoint &pos,
    const wxSize &size,
    const wxString &name
)
    : wxFileDialog(parent, message, default_dir, default_file, wildcard, style, pos, size, name)
    , checkbox_label(checkbox_label)
{
    if (checkbox_label.IsEmpty()) {
        return;
    }

    SetExtraControlCreator(ExtraPanel::ctor);
}

bool CheckboxFileDialog::get_checkbox_value() const
{
    auto *extra_panel = dynamic_cast<ExtraPanel*>(GetExtraControl());
    return extra_panel != nullptr ? extra_panel->cbox->GetValue() : false;
}


WindowMetrics WindowMetrics::from_window(wxTopLevelWindow *window)
{
    WindowMetrics res;
    res.rect = window->GetScreenRect();
    res.maximized = window->IsMaximized();
    return res;
}

boost::optional<WindowMetrics> WindowMetrics::deserialize(const std::string &str)
{
    std::vector<std::string> metrics_str;
    metrics_str.reserve(5);

    if (!unescape_strings_cstyle(str, metrics_str) || metrics_str.size() != 5) {
        return boost::none;
    }

    int metrics[5];
    try {
        for (size_t i = 0; i < 5; i++) {
            metrics[i] = boost::lexical_cast<int>(metrics_str[i]);
        }
    } catch(const boost::bad_lexical_cast &) {
        return boost::none;
    }

    if ((metrics[4] & ~1) != 0) {    // Checks if the maximized flag is 1 or 0
        metrics[4] = 0;
    }

    WindowMetrics res;
    res.rect = wxRect(metrics[0], metrics[1], metrics[2], metrics[3]);
    res.maximized = metrics[4] != 0;

    return res;
}

void WindowMetrics::sanitize_for_display(const wxRect &screen_rect)
{
    rect = rect.Intersect(screen_rect);

    // Prevent the window from going too far towards the right and/or bottom edge
    // It's hardcoded here that the threshold is 80% of the screen size
    rect.x = std::min(rect.x, screen_rect.x + 4*screen_rect.width/5);
    rect.y = std::min(rect.y, screen_rect.y + 4*screen_rect.height/5);
}

std::string WindowMetrics::serialize() const
{
    return (boost::format("%1%; %2%; %3%; %4%; %5%")
        % rect.x
        % rect.y
        % rect.width
        % rect.height
        % static_cast<int>(maximized)
    ).str();
}

std::ostream& operator<<(std::ostream &os, const WindowMetrics& metrics)
{
    return os << '(' << metrics.serialize() << ')';
}


}
}
