#pragma once

#include "Resource.h"

class CVideoProcPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CVideoProcPage)

public:
	CVideoProcPage();
	virtual ~CVideoProcPage();

	// Dialog Data
	enum { IDD = IDD_PROP_VIDEO_PROC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};

class CCameraCtrlPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CCameraCtrlPage)

public:
	CCameraCtrlPage();
	virtual ~CCameraCtrlPage();

	// Dialog Data
	enum { IDD = IDD_PROP_CAMERA_CTRL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};

class CVulpesCameraDlg
{
public:
	CVulpesCameraDlg(HWND pParent = nullptr);
	~CVulpesCameraDlg();
	enum { IDD = IDD_DIALOG_CAMERA };

public:
	HWND m_hWnd;
	HWND m_hWndParent;
	CPropertySheet m_sheet;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

public:
	static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	INT DoModal();
};

