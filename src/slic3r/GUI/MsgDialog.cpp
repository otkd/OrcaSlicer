#include "MsgDialog.hpp"

#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/statbmp.h>
#include <wx/scrolwin.h>
#include <wx/clipbrd.h>
#include <wx/checkbox.h>
#include <wx/html/htmlwin.h>

#include <boost/algorithm/string/replace.hpp>

#include "libslic3r/libslic3r.h"
#include "libslic3r/Utils.hpp"
#include "GUI.hpp"
#include "I18N.hpp"
//#include "ConfigWizard.hpp"
#include "wxExtensions.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "GUI_App.hpp"

#define DESIGN_INPUT_SIZE wxSize(FromDIP(100), -1)

namespace Slic3r {
namespace GUI {

MsgDialog::MsgDialog(wxWindow *parent, const wxString &title, const wxString &headline, long style, wxBitmap bitmap)
	: DPIDialog(parent ? parent : dynamic_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY, title, wxDefaultPosition, wxSize(360, -1),wxDEFAULT_DIALOG_STYLE)
	, boldfont(wxGetApp().normal_font())
	, content_sizer(new wxBoxSizer(wxVERTICAL))
	, btn_sizer(new wxBoxSizer(wxHORIZONTAL))
{
	boldfont.SetWeight(wxFONTWEIGHT_BOLD);
    SetBackgroundColour(0xFFFFFF);
    SetFont(wxGetApp().normal_font());
    CenterOnParent();

    auto *main_sizer = new wxBoxSizer(wxVERTICAL);
	auto *topsizer = new wxBoxSizer(wxHORIZONTAL);
	auto *rightsizer = new wxBoxSizer(wxVERTICAL);

	//auto *headtext = new wxStaticText(this, wxID_ANY, headline);
	//headtext->SetFont(boldfont);
 //   headtext->Wrap(CONTENT_WIDTH*wxGetApp().em_unit());
	//rightsizer->Add(headtext);
	//rightsizer->AddSpacer(VERT_SPACING);

	rightsizer->Add(content_sizer, 1, wxEXPAND);

	logo = new wxStaticBitmap(this, wxID_ANY, bitmap.IsOk() ? bitmap : wxNullBitmap);
    topsizer->Add(LOGO_SPACING, 0, 0, wxEXPAND, 0);
	topsizer->Add(logo, 0, wxTOP, BORDER);
    topsizer->Add(LOGO_GAP, 0, 0, wxEXPAND, 0);
	topsizer->Add(rightsizer, 1, wxTOP | wxEXPAND, BORDER);

    btn_sizer->AddStretchSpacer();

    main_sizer->Add(topsizer, 1, wxEXPAND);

    m_dsa_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(m_dsa_sizer,1,wxEXPAND,0);
    btn_sizer->Add(0, 0, 1, wxEXPAND, 5);
    main_sizer->Add(btn_sizer, 0, wxBOTTOM | wxRIGHT | wxEXPAND, BORDER);

    apply_style(style);
	SetSizerAndFit(main_sizer);
    wxGetApp().UpdateDlgDarkUI(this);
}

 MsgDialog::~MsgDialog()
{
    for (auto mb : m_buttons) { delete mb.second->buttondata ; delete mb.second; }
}

void MsgDialog::show_dsa_button(wxString const &title)
{
    m_checkbox_dsa = new CheckBox(this);
    m_dsa_sizer->Add(m_checkbox_dsa, 0, wxALL | wxALIGN_CENTER, FromDIP(2));
    m_checkbox_dsa->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent& e) {
        auto event = wxCommandEvent(EVT_CHECKBOX_CHANGE);
        event.SetInt(m_checkbox_dsa->GetValue()?1:0);
        event.SetEventObject(this);
        wxPostEvent(this, event);
        e.Skip();
    });

    auto  m_text_dsa = new wxStaticText(this, wxID_ANY, title.IsEmpty() ? _L("Don't show again") : title, wxDefaultPosition, wxDefaultSize, 0);
    m_dsa_sizer->Add(m_text_dsa, 0, wxALL | wxALIGN_CENTER, FromDIP(2));
    m_text_dsa->SetFont(::Label::Body_13);
    m_text_dsa->SetForegroundColour(StateColor::darkModeColorFor(wxColour("#323A3D")));
    btn_sizer->Layout();
    //Fit();
}

bool MsgDialog::get_checkbox_state() 
{
    if (m_checkbox_dsa) {
        return m_checkbox_dsa->GetValue();
    }
    return false;
}

void MsgDialog::on_dpi_changed(const wxRect &suggested_rect) 
 {
     if (m_buttons.size() > 0) {
         MsgButtonsHash::iterator i = m_buttons.begin();

         while (i != m_buttons.end()) {
             MsgButton *bd   = i->second;
             wxSize     bsize;


             switch (bd->buttondata->type) {
                case ButtonSizeNormal:bsize = MSG_DIALOG_BUTTON_SIZE;break;
                case ButtonSizeMiddle: bsize = MSG_DIALOG_MIDDLE_BUTTON_SIZE; break;
                case ButtonSizeLong: bsize = MSG_DIALOG_LONG_BUTTON_SIZE; break;
                default: break;
             }

             bd->buttondata->button->SetMinSize(bsize);
             i++;
         }
     }
 }

void MsgDialog::SetButtonLabel(wxWindowID btn_id, const wxString& label, bool set_focus/* = false*/) 
{
    if (Button* btn = get_button(btn_id)) {
        btn->SetLabel(label);
        if (set_focus)
            btn->SetFocus();
    }
}

Button* MsgDialog::add_button(wxWindowID btn_id, bool set_focus /*= false*/, const wxString& label/* = wxString()*/)
{
    Button* btn = new Button(this, label, "", 0, 0, btn_id);
    ButtonSizeType type;

    if (label.length() < 5) {
        type = ButtonSizeNormal;
        btn->SetMinSize(MSG_DIALOG_BUTTON_SIZE); }
    else if (label.length() >= 5 && label.length() < 8) {
        type = ButtonSizeMiddle;
        btn->SetMinSize(MSG_DIALOG_MIDDLE_BUTTON_SIZE);
    } 
    else if (label.length() >= 8 && label.length() < 12) {
        type = ButtonSizeMiddle;
        btn->SetMinSize(MSG_DIALOG_LONG_BUTTON_SIZE);
    } else {
        type = ButtonSizeLong;
        btn->SetMinSize(MSG_DIALOG_LONGER_BUTTON_SIZE);
    }
    
    btn->SetCornerRadius(FromDIP(12));
    StateColor btn_bg_green(
        std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal)
    );

    StateColor btn_bd_green(
        std::pair<wxColour, int>(wxColour(0, 150, 136), StateColor::Normal)
    );

    StateColor btn_text_green(
        std::pair<wxColour, int>(wxColour(255, 255, 254), StateColor::Normal)
    );

    StateColor btn_bg_white(
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal)
    );

    StateColor btn_bd_white(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );

    StateColor btn_text_white(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );

    if (btn_id == wxID_OK || btn_id == wxID_YES) {
        btn->SetBackgroundColor(btn_bg_green);
        btn->SetBorderColor(btn_bd_green);
        btn->SetTextColor(btn_text_green);
    }

    if (btn_id == wxID_CANCEL || btn_id == wxID_NO) {
        btn->SetBackgroundColor(btn_bg_white);
        btn->SetBorderColor(btn_bd_white);
        btn->SetTextColor(btn_text_white);
    }

    if (set_focus)
        btn->SetFocus();
    btn_sizer->Add(btn, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, BTN_SPACING);
    btn->Bind(wxEVT_BUTTON, [this, btn_id](wxCommandEvent&) { EndModal(btn_id); });

    MsgButton *mb = new MsgButton;
    ButtonData *bd = new ButtonData;

    bd->button = btn;
    bd->type   = type;

    mb->id        = wxString::Format("%d", m_buttons.size());
    mb->buttondata = bd;
    m_buttons[ wxString::Format("%d", m_buttons.size())] = mb;
    return btn;
};

Button* MsgDialog::get_button(wxWindowID btn_id){
    return static_cast<Button*>(FindWindowById(btn_id, this));
}

void MsgDialog::apply_style(long style)
{
    if (style & wxOK)       add_button(wxID_OK, true, _L("OK"));
    if (style & wxYES)      add_button(wxID_YES, true, _L("Yes"));
    if (style & wxNO)       add_button(wxID_NO, false,_L("No"));
    if (style & wxCANCEL)   add_button(wxID_CANCEL, false, _L("Cancel"));

    logo->SetBitmap( create_scaled_bitmap(style & wxAPPLY        ? "completed" :
                                          style & wxICON_WARNING        ? "obj_warning" :
                                          style & wxICON_INFORMATION    ? "info"        :
                                          style & wxICON_QUESTION       ? "question"    : "OrcaSlicer", this, 64, style & wxICON_ERROR));
}

void MsgDialog::finalize()
{
    wxGetApp().UpdateDlgDarkUI(this);
    Fit();
    CenterOnParent();
}


// Text shown as HTML, so that mouse selection and Ctrl-V to copy will work.
static void add_msg_content(wxWindow* parent, wxBoxSizer* content_sizer, wxString msg, bool monospaced_font = false, bool is_marked_msg = false)
{
    wxHtmlWindow* html = new wxHtmlWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
    html->SetBackgroundColour(StateColor::darkModeColorFor(*wxWHITE));

    // count lines in the message
    int msg_lines = 0;
    if (!monospaced_font) {
        int line_len = 55;// count of symbols in one line
        int start_line = 0;
        for (auto i = msg.begin(); i != msg.end(); ++i) {
            if (*i == '\n') {
                int cur_line_len = i - msg.begin() - start_line;
                start_line = i - msg.begin();
                if (cur_line_len == 0 || line_len > cur_line_len)
                    msg_lines++;
                else
                    msg_lines += std::lround((double)(cur_line_len) / line_len);
            }
        }
        msg_lines++;
    }

    wxFont      font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    wxFont      monospace = wxGetApp().code_font();
    wxColour    text_clr = wxGetApp().get_label_clr_default();
    wxColour    bgr_clr = parent->GetBackgroundColour(); //wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    auto        text_clr_str = wxString::Format(wxT("#%02X%02X%02X"), text_clr.Red(), text_clr.Green(), text_clr.Blue());
    auto        bgr_clr_str = wxString::Format(wxT("#%02X%02X%02X"), bgr_clr.Red(), bgr_clr.Green(), bgr_clr.Blue());
    const int   font_size = font.GetPointSize();
    int         size[] = { font_size, font_size, font_size, font_size, font_size, font_size, font_size };
    html->SetFonts(font.GetFaceName(), monospace.GetFaceName(), size);
    html->SetBorders(2);

    // calculate html page size from text
    wxSize page_size;
    int em = wxGetApp().em_unit();
    if (!wxGetApp().mainframe) {
        // If mainframe is nullptr, it means that GUI_App::on_init_inner() isn't completed 
        // (We just show information dialog about configuration version now)
        // And as a result the em_unit value wasn't created yet
        // So, calculate it from the scale factor of Dialog
#if defined(__WXGTK__)
        // Linux specific issue : get_dpi_for_window(this) still doesn't responce to the Display's scale in new wxWidgets(3.1.3).
        // So, initialize default width_unit according to the width of the one symbol ("m") of the currently active font of this window.
        em = std::max<size_t>(10, parent->GetTextExtent("m").x - 1);
#else
        double scale_factor = (double)get_dpi_for_window(parent) / (double)DPI_DEFAULT;
        em = std::max<size_t>(10, 10.0f * scale_factor);
#endif // __WXGTK__
    }

    // if message containes the table
    if (msg.Contains("<tr>")) {
        int lines = msg.Freq('\n') + 1;
        int pos = 0;
        while (pos < (int)msg.Len() && pos != wxNOT_FOUND) {
            pos = msg.find("<tr>", pos + 1);
            lines += 2;
        }
        int page_height = std::min(int(font.GetPixelSize().y+2) * lines, 68 * em);
        page_size = wxSize(68 * em, page_height);
    }
    else {
        wxClientDC dc(parent);
        wxSize msg_sz = dc.GetMultiLineTextExtent(msg);
        page_size = wxSize(std::min(msg_sz.GetX() + 2 * em, 68 * em),
                           std::min(msg_sz.GetY() + 2 * em, 68 * em));
    }
    html->SetMinSize(page_size);

    std::string msg_escaped = xml_escape(msg.ToUTF8().data(), is_marked_msg);
    boost::replace_all(msg_escaped, "\r\n", "<br>");
    boost::replace_all(msg_escaped, "\n", "<br>");
    if (monospaced_font)
        // Code formatting will be preserved. This is useful for reporting errors from the placeholder parser.
        msg_escaped = std::string("<pre><code>") + msg_escaped + "</code></pre>";
    html->SetPage("<html><body bgcolor=\"" + bgr_clr_str + "\"><font color=\"" + text_clr_str + "\">" + wxString::FromUTF8(msg_escaped.data()) + "</font></body></html>");
    content_sizer->Add(html, 1, wxEXPAND);
    wxGetApp().UpdateDarkUIWin(html);
}

// ErrorDialog

ErrorDialog::ErrorDialog(wxWindow *parent, const wxString &msg, bool monospaced_font)
    : MsgDialog(parent, wxString::Format(_(L("%s error")), SLIC3R_APP_FULL_NAME), 
                        wxString::Format(_(L("%s has encountered an error")), SLIC3R_APP_FULL_NAME), wxOK)
	, msg(msg)
{
    add_msg_content(this, content_sizer, msg, monospaced_font);

	// Use a small bitmap with monospaced font, as the error text will not be wrapped.
	logo->SetBitmap(create_scaled_bitmap("OrcaSlicer_192px_grayscale.png", this, monospaced_font ? 48 : /*1*/84));

    SetMaxSize(wxSize(-1, CONTENT_MAX_HEIGHT*wxGetApp().em_unit()));

    finalize();
}

// WarningDialog

WarningDialog::WarningDialog(wxWindow *parent,
                             const wxString& message,
                             const wxString& caption/* = wxEmptyString*/,
                             long style/* = wxOK*/)
    : MsgDialog(parent, caption.IsEmpty() ? wxString::Format(_L("%s warning"), SLIC3R_APP_FULL_NAME) : caption, 
                        wxString::Format(_L("%s has a warning")+":", SLIC3R_APP_FULL_NAME), style)
{
    add_msg_content(this, content_sizer, message);
    finalize();
}

#if 1
// MessageDialog

MessageDialog::MessageDialog(wxWindow* parent,
    const wxString& message,
    const wxString& caption/* = wxEmptyString*/,
    long style/* = wxOK*/)
    : MsgDialog(parent, caption.IsEmpty() ? wxString::Format(_L("%s info"), SLIC3R_APP_FULL_NAME) : caption, wxEmptyString, style)
{
    add_msg_content(this, content_sizer, message);
    finalize();
    wxGetApp().UpdateDlgDarkUI(this);
}


// RichMessageDialog

RichMessageDialog::RichMessageDialog(wxWindow* parent,
    const wxString& message,
    const wxString& caption/* = wxEmptyString*/,
    long style/* = wxOK*/)
    : MsgDialog(parent, caption.IsEmpty() ? wxString::Format(_L("%s info"), SLIC3R_APP_FULL_NAME) : caption, wxEmptyString, style)
{
    add_msg_content(this, content_sizer, message);

    m_checkBox = new wxCheckBox(this, wxID_ANY, m_checkBoxText);
    wxGetApp().UpdateDarkUI(m_checkBox);
    m_checkBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { m_checkBoxValue = m_checkBox->GetValue(); });

    btn_sizer->Insert(0, m_checkBox, wxALIGN_CENTER_VERTICAL);

    finalize();
}

int RichMessageDialog::ShowModal()
{
    if (m_checkBoxText.IsEmpty())
        m_checkBox->Hide();
    else
        m_checkBox->SetLabelText(m_checkBoxText);
    Layout();

    return wxDialog::ShowModal();
}
#endif

// InfoDialog
InfoDialog::InfoDialog(wxWindow* parent, const wxString &title, const wxString& msg, bool is_marked_msg/* = false*/, long style/* = wxOK | wxICON_INFORMATION*/)
    : MsgDialog(parent, wxString::Format(_L("%s information"), SLIC3R_APP_FULL_NAME), title, style)
	, msg(msg)
{
    add_msg_content(this, content_sizer, msg, false, is_marked_msg);
    finalize();
}

// InfoDialog
DownloadDialog::DownloadDialog(wxWindow *parent, const wxString &msg, const wxString &title, bool is_marked_msg /* = false*/, long style /* = wxOK | wxICON_INFORMATION*/)
    : MsgDialog(parent, title, msg, style), msg(msg)
{
    add_button(wxID_YES, true, _L("Download"));
    add_button(wxID_CANCEL, true, _L("Skip"));
    
    finalize();
}


void DownloadDialog::SetExtendedMessage(const wxString &extendedMessage) 
{
    add_msg_content(this, content_sizer, msg + "\n" + extendedMessage, false, false);
    Layout();
    Fit();
}

}} // namespace Slic3r::GUI
