#include "stdafx.h"
#include "VulpesCamera.h"
#include "VulpesVideoPlay.h"
#include "VulpesCameraDlg.h"
#include "afxdialogex.h"

extern CCameraWndData *_pData;

IMPLEMENT_DYNAMIC(CVideoProcPage, CPropertyPage)

CVideoProcPage::CVideoProcPage()
	: CPropertyPage(CVideoProcPage::IDD)
{
}

CVideoProcPage::~CVideoProcPage()
{
}

void CVideoProcPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVideoProcPage, CPropertyPage)
END_MESSAGE_MAP()


IMPLEMENT_DYNAMIC(CCameraCtrlPage, CPropertyPage)

CCameraCtrlPage::CCameraCtrlPage()
	: CPropertyPage(CCameraCtrlPage::IDD)
{
}

CCameraCtrlPage::~CCameraCtrlPage()
{
}

void CCameraCtrlPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCameraCtrlPage, CPropertyPage)
END_MESSAGE_MAP()

CVulpesCameraDlg::CVulpesCameraDlg(HWND pParent)
{
	m_hWndParent = pParent;
}

CVulpesCameraDlg::~CVulpesCameraDlg()
{
}

void CVulpesCameraDlg::DoDataExchange(CDataExchange* pDX)
{
}

BOOL CVulpesCameraDlg::OnInitDialog()
{
	return TRUE;
}

INT CVulpesCameraDlg::DoModal()
{
	HMODULE hModule = GetModuleHandleA("VulpesCamera.dll");
	return (INT)DialogBox(hModule, MAKEINTRESOURCE(CVulpesCameraDlg::IDD), m_hWndParent, DialogProc);
}


HWND CPropertySheet_Create(CWnd* pParentWnd, DWORD dwStyle, DWORD dwExStyle)
{
	PROPSHEETHEADER m_psh;
	memset(&m_psh, 0, sizeof(m_psh));
	m_psh.dwSize = sizeof(m_psh);
	m_psh.dwFlags = PSH_PROPSHEETPAGE;
	m_psh.pszCaption = NULL;
	m_psh.nStartPage = 0;

	_AFX_THREAD_STATE* pState = AfxGetThreadState();

	if (dwStyle == (DWORD)-1) {
		pState->m_dwPropStyle = DS_MODALFRAME | DS_3DLOOK | DS_CONTEXTHELP |
			DS_SETFONT | WS_POPUP | WS_VISIBLE | WS_CAPTION;
		pState->m_dwPropStyle |= WS_SYSMENU;
	}
	else {
		pState->m_dwPropStyle = dwStyle;
	}
	pState->m_dwPropExStyle = dwExStyle;
	free((void*)m_psh.ppsp);
	m_psh.ppsp = NULL;

	int nBytes = sizeof(PROPSHEETPAGE) * 2;
	PROPSHEETPAGE* ppsp = (PROPSHEETPAGE*)malloc(nBytes);
	BYTE* ppspOrigByte = reinterpret_cast<BYTE*>(ppsp);
	if (ppsp == NULL) {
		AfxThrowMemoryException();
	}
	memset(ppsp, 0, nBytes);
	BYTE* pPropSheetPagesArrEnd = ppspOrigByte + nBytes;
	ENSURE(pPropSheetPagesArrEnd >= ppspOrigByte);
	BOOL bWizard = (m_psh.dwFlags & (PSH_WIZARD | PSH_WIZARD97));
	LPCTSTR lpszTemplateName = MAKEINTRESOURCE(IDD_PROP_VIDEO_PROC);
	PROPSHEETPAGE &m_psp = ppsp[0];
	m_psp.dwFlags = PSP_USECALLBACK;
	m_psp.hInstance = GetModuleHandleA("VulpesCamera.dll");
	m_psp.pszTemplate = lpszTemplateName;
	m_psp.pfnDlgProc = CVulpesCameraDlg::DialogProc;
	m_psp.lParam = (LPARAM)NULL;
	m_psp.pfnCallback = NULL;

	m_psh.ppsp = ppsp;
	m_psh.nPages = 2;
	m_psh.dwFlags |= (PSH_MODELESS | PSH_USECALLBACK);
	m_psh.pfnCallback = NULL;
	m_psh.hwndParent = pParentWnd->GetSafeHwnd();

	HWND hWnd = (HWND)PropertySheet(&m_psh);

#ifdef _DEBUG
	DWORD dwError = ::GetLastError();
#endif

	if (hWnd == (HWND)-1) {
		return FALSE;
	}
	return hWnd;
}

void SetTrackBarRange(HWND hWndDlg, UINT nID, INT from, INT to, INT def)
{
	HWND hWnd = GetDlgItem(hWndDlg, nID);
	if (hWnd == NULL)
		return;
	SendMessage(hWnd, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(from, to));
	SendMessage(hWnd, TBM_SETPOS, (WPARAM)TRUE, def);
}

void SetEditorText(HWND hWndDlg, UINT nID, INT value)
{
	HWND hWnd = GetDlgItem(hWndDlg, nID);
	if (hWnd == NULL)
		return;
	CString str;
	str.Format(_T("%d"), value);
	SetWindowText(hWnd, str);
}

static cv::Mat proc_brightness(cv::Mat src, LPVOID pData)
{
	PIMG_PROC proc = (PIMG_PROC)pData;
	double brightness = (proc->value - proc->from) / (double)(proc->defValue - proc->from);
	cv::Mat channels[3];
	cv::split(src, channels);
	cv::Mat z = cv::Mat::zeros(channels[0].size(), channels[0].type());
	cv::addWeighted(channels[0], brightness, z, 0, 0, channels[0]);
	cv::merge(channels, 3, src);
	cvtColor(src, src, CV_YUV2BGR);
	return src;
}

static cv::Mat proc_contrast(cv::Mat src, LPVOID pData)
{
	PIMG_PROC proc = (PIMG_PROC)pData;
	double alpha = (proc->value - proc->from) / (double)(proc->defValue - proc->from);
	int beta = 0; // brightness

	for (int y = 0; y < src.rows; y++) {
		for (int x = 0; x < src.cols; x++) {
			for (int c = 0; c < 3; c++) {
				src.at<cv::Vec3b>(y, x)[c] = cv::saturate_cast<uchar>(128 + alpha * (src.at<cv::Vec3b>(y, x)[c] - 128) + beta);
			}
		}
	}

	return src;
}

static cv::Mat proc_hue(cv::Mat src, LPVOID pData)
{
	return src;
}

static cv::Mat proc_saturation(cv::Mat src, LPVOID pData)
{
	PIMG_PROC proc = (PIMG_PROC)pData;
	double saturation = (proc->value - proc->from) * 10.0;
	cv::cvtColor(src, src, CV_BGR2HSV);
	cv::Mat channels[3];
	cv::split(src, channels);
	// Adjust the saturation
	cv::Mat z = cv::Mat::zeros(channels[1].size(), channels[1].type());
	// dst = alpha*src1 + beta*src2 + gamma
	cv::addWeighted(channels[1], 1.0, z, 0, saturation, channels[1]);
	cv::merge(channels, 3, src);
	cv::cvtColor(src, src, CV_HSV2BGR);
	return src;
}

static cv::Mat proc_sharpness(cv::Mat src, LPVOID pData)
{
	PIMG_PROC proc = (PIMG_PROC)pData;
	cv::Mat src0;
	src.copyTo(src0);
	for (int i = 0; i < proc->value; i++) {
		cv::Mat dst;
		CV_Assert(dst.depth() == CV_8U);  // accept only uchar images

		const int nChannels = src0.channels();
		dst.create(src0.size(), src0.type());

		for (int j = 1; j < src0.rows - 1; ++j)
		{
			const uchar* previous = src0.ptr<uchar>(j - 1);
			const uchar* current = src0.ptr<uchar>(j);
			const uchar* next = src0.ptr<uchar>(j + 1);

			uchar* output = dst.ptr<uchar>(j);

			for (int i = nChannels; i < nChannels*(src0.cols - 1); ++i)
			{
				*output++ = cv::saturate_cast<uchar>(5 * current[i]
					- current[i - nChannels] - current[i + nChannels] - previous[i] - next[i]);
			}
		}

		dst.row(0).setTo(cv::Scalar(0));
		dst.row(dst.rows - 1).setTo(cv::Scalar(0));
		dst.col(0).setTo(cv::Scalar(0));
		dst.col(dst.cols - 1).setTo(cv::Scalar(0));

		dst.copyTo(src0);
	}

	return src0;
}

static cv::Mat proc_gamma(cv::Mat src, LPVOID pData)
{
	return src;
}

static cv::Mat proc_white_balance(cv::Mat src, LPVOID pData)
{
	return src;
}

static cv::Mat proc_backlight_comp(cv::Mat src, LPVOID pData)
{
	return src;
}

static cv::Mat proc_gain(cv::Mat src, LPVOID pData)
{
	return src;
}

struct {
	UINT nIDSlider;
	INT from;
	INT to;
	INT defValue;
	UINT nIDEditor;
	UINT nIDCheck;
	ImgProc proc;
	INT nProp;
}
nDefaults[] = {
	{ IDC_SLID_BRIGHTNESS,      -10,   10,    0, IDC_EDT_BRIGHTNESS,        IDC_CHK_BRIGHTNESS,     proc_brightness,    0 }, //CV_CAP_PROP_BRIGHTNESS },
	{ IDC_SLID_CONTRAST,          0,   20,   10, IDC_EDT_CONTRAST,          IDC_CHK_CONTRAST,       proc_contrast,      0 }, //CV_CAP_PROP_CONTRAST },
	{ IDC_SLID_HUE,             -10,   10,    0, IDC_EDT_HUE,               IDC_CHK_HUE,            proc_hue,           0 }, //CV_CAP_PROP_HUE },
	{ IDC_SLID_SATURATION,        0,   10,    0, IDC_EDT_SATURATION,        IDC_CHK_SATURATION,     proc_saturation,    0 }, //CV_CAP_PROP_SATURATION },
	{ IDC_SLID_SHARPNESS,         0,   10,    0, IDC_EDT_SHARPNESS,         IDC_CHK_SHARPNESS,      proc_sharpness,     0 }, //CV_CAP_PROP_SHARPNESS },
	{ IDC_SLID_GAMMA,           100,  200,  130, IDC_EDT_GAMMA,             IDC_CHK_GAMMA,          proc_gamma,         0 }, //CV_CAP_PROP_GAMMA },
	{ IDC_SLID_WHITE_BALANCE,  2800, 6500, 6500, IDC_EDT_WHITE_BALANCE,     IDC_CHK_WHITE_BALANCE,  proc_white_balance, 0 },
	{ IDC_SLID_BACKLIGHT_COMP,    0,    2,    1, IDC_EDT_BACKLIGHT_COMP,    IDC_CHK_BACKLIGHT_COMP, proc_backlight_comp, 0 }, //CV_CAP_PROP_BACKLIGHT },
	{ IDC_SLID_GAIN,              0,   20,    0, IDC_EDT_GAIN,              IDC_CHK_GAIN,           proc_gain,          0 }, //CV_CAP_PROP_GAIN  },
	{ 0, 0, 0, 0, 0, 0, NULL },
};

INT_PTR CALLBACK CVulpesCameraDlg::DialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	NMHDR *lpnmh;

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		_pData->m_csResult.Lock();
		for (i = 0; nDefaults[i].nIDSlider > 0; i++) {
			INT curValue = nDefaults[i].nProp > 0 ? (INT)_pData->cvCapture.get(CV_CAP_INTELPERC_IMAGE_GENERATOR | nDefaults[i].nProp) : nDefaults[i].defValue;
			SetTrackBarRange(hWndDlg, nDefaults[i].nIDSlider, nDefaults[i].from, nDefaults[i].to, curValue);
			SetEditorText(hWndDlg, nDefaults[i].nIDEditor, curValue);
		}
		_pData->m_csResult.Unlock();
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:

		case IDCANCEL:
			EndDialog(hWndDlg, wParam);
			return TRUE;
		}
		break;
	case WM_NOTIFY:
		lpnmh = (LPNMHDR)lParam;
		switch (lpnmh->code)
		{
		case NM_RELEASEDCAPTURE:
			_pData->m_csResult.Lock();
			for (i = 0; nDefaults[i].nIDSlider > 0; i++) {
				if (lpnmh->idFrom == nDefaults[i].nIDSlider) {
					INT value = (INT)SendMessage(lpnmh->hwndFrom, TBM_GETPOS, 0, 0);
					SetEditorText(hWndDlg, nDefaults[i].nIDEditor, value);
					if (nDefaults[i].nProp == 0) {
						if (value != nDefaults[i].defValue)
							register_img_proc(nDefaults[i].proc,
								nDefaults[i].from, nDefaults[i].to, nDefaults[i].defValue, value);
						else
							deregister_img_proc(nDefaults[i].proc);
					}
					else {
						_pData->cvCapture.set(CV_CAP_INTELPERC_IMAGE_GENERATOR | nDefaults[i].nProp, value);
					}
					break;
				}
			}
			_pData->m_csResult.Unlock();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return NULL;
}
