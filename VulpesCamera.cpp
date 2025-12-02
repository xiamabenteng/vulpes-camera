#include "stdafx.h"
#include "VulpesCamera.h"
#include "VulpesVideoPlay.h"
#include "VulpesCameraDlg.h"
#include "reg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CCameraWndData::CCameraWndData()
{
	pCallbackData = NULL;
	pFunctionCallbackVideoPlay = NULL;
	pFunctionCallbackAudioPlay = NULL;

	nDeviceId = 0;
	memset(szDeviceName, 0, sizeof(szDeviceName));
	memset(szVideoFile, 0, sizeof(szVideoFile));
	nFrameCount = 0;
	dblVideoDuration = 0.0;
	dblStart = 0.0;
	nBPP = 24;
	nRotation = 0;

	bPreviewing = FALSE;

	bCapturing = FALSE;
	szCaptureFile[0] = '\0';
	bCapturingPaused = FALSE;
	dwCaptureStarted = 0;

	m_pVideoPlay = NULL;
	m_hVideoPlayThread = NULL;
	m_nVideoPlayThreadId = 0;

	bMpg = FALSE;

	nUseTimeLimit = 0;
	dwTimeLimit = 0;

	nUseFrameRate = 0;
	dblFrameRate = 12.0;

	dwCaptureFileAllocSize = 0;

	dblSampleTime = 0.0;

	m_fPlaySpeed = 1.0f;
	m_fInputPlaySpeed = 1.0f;
	m_dPlaySpeedK = 1.0;

	hWndCapture = NULL;
	m_dAudioDelay = 0;
	fpsIP = 0.f;
	bIP = false;
	szRegKey[0] = '\0';
	bCaptureCC = false;
	m_hWndNotify = NULL;
	m_nInplaceVideo = 0;
	m_fBaseFps = 25.0f;
}

CCameraWndData *_pData = NULL;
static int _nCAMCapWndSerial = 0;
static int _nCameraCnt = 0;


CVulpesCamera::CVulpesCamera()
{
	_nCAMCapWndSerial++;

	if (_pData == NULL) {
		_pData = new CCameraWndData();

		CObject *pObject = RUNTIME_CLASS(CVulpesVideoPlay)->CreateObject();
		ASSERT(pObject->IsKindOf(RUNTIME_CLASS(CVulpesVideoPlay)));
		_pData->m_pVideoPlay = (CVulpesVideoPlay *)pObject;
		_pData->m_pVideoPlay->CreateThreadEx();
		_pData->m_pVideoPlay->setData(_pData);
	}

	m_pCapture = _pData;
}

CVulpesCamera::~CVulpesCamera()
{
	Stop();
	_nCAMCapWndSerial--;
	if (_nCAMCapWndSerial == 0)
	{
		CCameraWndData *pData = (CCameraWndData *)m_pCapture;

		pData->m_pVideoPlay->PostThreadMessage(AM_PREPARE_NEXT, (WPARAM)this, 0);
		TerminateThread(pData->m_pVideoPlay->m_hThread, 0);
		WaitForSingleObject(pData->m_pVideoPlay->m_hThread, INFINITE);
		pData->m_pVideoPlay->Delete();
		if (pData->hWndCapture != NULL)
			DestroyWindow(pData->hWndCapture);

		delete pData;
		m_pCapture = NULL;
	}
}


BOOL CVulpesCamera::ShowWindow(int nCmdShow)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	BOOL result = ::ShowWindow(pData->hWndCapture, nCmdShow);
	result = TRUE;
	return result;
}

BOOL CVulpesCamera::Create(LPCWSTR lpszClassName, LPCWSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	BOOL result;
	pData->pParentWnd = pParentWnd;
	pData->hWndCapture = capCreateCaptureWindow(lpszWindowName, dwStyle,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		pParentWnd->m_hWnd, nID);
	result = pData->hWndCapture != NULL;
	return result;
}

void CVulpesCamera::MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	::MoveWindow(pData->hWndCapture, x, y, nWidth, nHeight, bRepaint);
}

void CVulpesCamera::MoveWindow(LPCRECT lpRect, BOOL bRepaint)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	::MoveWindow(pData->hWndCapture,
		lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top,
		bRepaint);
}


HWND CVulpesCamera::GetWindow(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	HWND result = pData->hWndCapture;
	return result;
}

void CVulpesCamera::SetRegistryKey(LPCWSTR key)
{

	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	wcscpy_s(pData->szRegKey, MAX_PATH, key);
}

void CVulpesCamera::SetRegistryKeyGroup(LPCWSTR key)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
}

void CVulpesCamera::SetTimeLimit(int nUse, DWORD dwLimit)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->nUseTimeLimit = nUse;
	pData->dwTimeLimit = dwLimit;
}

void CVulpesCamera::GetTimeLimit(int &nUse, DWORD &dwLimit)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	nUse = pData->nUseTimeLimit;
	dwLimit = pData->dwTimeLimit;
}


void CVulpesCamera::SetFrameRate(int nUse, double dwRate)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->cvCapture.set(CV_CAP_PROP_FPS, dwRate);
	pData->nUseFrameRate = nUse;
	pData->dblFrameRate = dwRate;
}

void CVulpesCamera::GetFrameRate(int &nUse, double &dRate)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	nUse = pData->nUseFrameRate;
	dRate = pData->dblFrameRate;
}


BOOL CVulpesCamera::SetCaptureFileAllocSize(WORD size)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->dwCaptureFileAllocSize = size;
	return TRUE;
}

WORD CVulpesCamera::GetCaptureFileAllocSize()
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	WORD result = (WORD)pData->dwCaptureFileAllocSize;
	return result;
}


BOOL CVulpesCamera::SetCaptureFile(LPCWSTR file)
{
	if (file == NULL) {
		return FALSE;
	}

	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	BOOL result = FALSE;
	if (wcsstr(file, L".") != NULL) {
		pData->szCaptureFile[0] = L'\0';
		wcscpy_s(pData->szCaptureFile, sizeof(pData->szCaptureFile) / sizeof(WCHAR), file);

		CStringA afile;
		afile = file;
		int ex = CV_FOURCC('X', 'V', 'I', 'D');
		cv::Size sz;

		pData->m_csResult.Lock();
		if (pData->cvCapture.isOpened())
			sz = cv::Size((int)pData->cvCapture.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
			(int)pData->cvCapture.get(CV_CAP_PROP_FRAME_HEIGHT));
		else
			sz = cv::Size(600, 480);

		double fps = pData->cvCapture.get(CV_CAP_PROP_FPS);
		if (fps == 0.0)
			fps = pData->dblFrameRate;
		if (pData->cvWriter.open((LPCSTR)afile, ex, fps, sz, true)) {
			pData->dwCaptureStarted = GetTickCount();
			result = TRUE;
		}
		else
			result = FALSE;

		pData->m_csResult.Unlock();
	}
	return result;
}


LPCWSTR CVulpesCamera::GetCaptureFile()
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	LPCWSTR result = pData->szCaptureFile;
	return result;
}


BOOL CVulpesCamera::SaveCaptureFile(LPCWSTR tachDstFile)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	return TRUE;
}


void CVulpesCamera::SetCaptureCC(bool cap)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->bCaptureCC = cap;
}

bool CVulpesCamera::GetCaptureCC()
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCaptureCC;
	return result;
}


void CVulpesCamera::CmdCapture(bool bPaused)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (!bPaused) {
		pData->m_csResult.Lock();
		if (pData->cvWriter.isOpened()) {
			pData->bCapturing = bPaused == false;
		}
		pData->m_csResult.Unlock();
	}
	pData->bCapturingPaused = bPaused != false;

}


bool CVulpesCamera::CanCmdCapture(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->nDeviceId >= 0 && !pData->bCapturing;

	return result;
}


void CVulpesCamera::CmdPreview(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (pData->nDeviceId >= 0) {
		pData->m_pVideoPlay->StartCamera();
	}
	pData->bPreviewing = true;
}


void CVulpesCamera::CmdPreviewStop(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (pData->nDeviceId >= 0) {
		pData->m_pVideoPlay->StopCamera();
		pData->nDeviceId = -1;
		pData->cvCapture.release();
	}
	pData->bPreviewing = FALSE;
}


bool CVulpesCamera::CanCmdPreview(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->bPreviewing == FALSE;
	return result;
}


bool CVulpesCamera::CanCmdPreviewStop(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->bPreviewing != FALSE;

	return result;
}


void CVulpesCamera::CmdCaptureStop(int nMode)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->bCapturing = FALSE;
	pData->bPreviewing = FALSE;

	pData->m_csResult.Lock();
	if (pData->cvWriter.isOpened())
	{
		pData->cvWriter.release();
	}
	pData->m_csResult.Unlock();
}


bool CVulpesCamera::CanCmdCaptureStop(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->bCapturing != FALSE;

	return result;
}


bool CVulpesCamera::CanCmdVideoStop(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->bMpg != FALSE;

	return result;
}


void CVulpesCamera::CmdVideoStop(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (pData->bMpg) {
		pData->m_pVideoPlay->StopAVI();
		pData->szVideoFile[0] = L'\0';
		pData->bMpg = FALSE;
	}

}


void CVulpesCamera::ChooseAudioFormat(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	DWORD pMetric;
	if (!acmMetrics(0, ACM_METRIC_MAX_SIZE_FORMAT, &pMetric)) {
		LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)new BYTE[pMetric];
		memset(pwfx, 0, pMetric);
		if (capGetAudioFormat(pData->hWndCapture, pwfx, (WORD)pMetric)) {
			ACMFORMATCHOOSEW pafmtc;
			memset(&pafmtc, 0, sizeof(ACMFORMATCHOOSEW));
			pafmtc.cbStruct = sizeof(ACMFORMATCHOOSEW);
			pafmtc.fdwStyle = ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;
			pafmtc.fdwEnum = ACM_FORMATENUMF_INPUT | ACM_FORMATENUMF_HARDWARE;
			pafmtc.hwndOwner = NULL;
			pafmtc.pwfx = (LPWAVEFORMATEX)pwfx;
			pafmtc.cbwfx = pMetric;
			if (!acmFormatChooseW(&pafmtc)) {
				capSetAudioFormat(pData->hWndCapture, pwfx, pMetric);
			}
		}
		delete[](BYTE *)pwfx;
	}
}

bool CVulpesCamera::CanChooseAudioFormat(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	CAPTUREPARMS s;
	if (capCaptureGetSetup(pData->hWndCapture, &s, sizeof(s)))
		result = s.fCaptureAudio != FALSE;
	else
		result = false;
	result = result && !pData->bCapturing;
	return result;
}

bool CVulpesCamera::CanSetCaptureAudio(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->nDeviceId >= 0 && !pData->bCapturing;
	//OutputDebugString(L"CanSetCaptureAudio\n");
	return result;
}

bool CVulpesCamera::CanSetCaptureCC(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCaptureCC;
	return result;
}

bool CVulpesCamera::CanSetFrameRate(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->nDeviceId >= 0 && !pData->bCapturing;
	return result;
}

bool CVulpesCamera::CanSetTimeLimit(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCaptureCC;
	return result;
}

bool CVulpesCamera::CanSetCaptureFileAllocSize(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->nDeviceId >= 0 && !pData->bCapturing;
	return result;
}

bool CVulpesCamera::CanSetCaptureFile(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->nDeviceId >= 0 && !pData->bCapturing;
	return result;
}

bool CVulpesCamera::CanSaveCaptureFile(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->nDeviceId >= 0 && !pData->bCapturing;
	return result;
}

bool CVulpesCamera::CanCmdMasterNo(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCaptureCC;
	return result;
}

bool CVulpesCamera::CanCmdMasterAudio(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCaptureCC;
	return result;
}

bool CVulpesCamera::CanCmdMasterVideo(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCaptureCC;
	return result;
}

bool CVulpesCamera::CanRTSP(void)
{
	bool result = false;
	return result;
}


bool CVulpesCamera::IsCapturing(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCapturing != FALSE;
	return result;
}


bool CVulpesCamera::IsPreviewing(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bPreviewing != FALSE && pData->bCapturing == FALSE;
	return result;
}


void CVulpesCamera::CmdDeviceVideo(int n)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (n < _nCameraCnt)
	{
		bool result = false;

		pData->m_csResult.Lock();

		_pData->dblFrameRate = 25;

		if (pData->cvCapture.isOpened())
			pData->cvCapture.release();
		result = pData->cvCapture.open(n);
		if (result) {
			if (pData->m_pVideoPlay->PrepareCamera()) {
				pData->nDeviceId = n;
				pData->szVideoFile[0] = L'\0';
				result = true;
			}
			else
				result = false;
		}

		pData->m_csResult.Unlock();
		return;
	}



	WCHAR reg[MAX_PATH], url[MAX_PATH];
	_stprintf_s(reg, 256, L"SOFTWARE\\ELSYS\\CamIP\\%0.2u", n - _nCameraCnt);
	GetProfileStringCmd(reg, L"url", url, sizeof(url));

	pData->m_csResult.Lock();
	if (url[0] != '\0') {
		wcscpy_s(pData->szVideoFile, MAX_PATH, url);

		Stop();

		pData->nFrameCount = 0;
		pData->dblVideoDuration = 0.0;
		CStringA filea;
		filea = url;

		pData->dblStart = 0;
		pData->m_pVideoPlay->PlayVideo();

		pData->bMpg = true;
		pData->bPreviewing = true;
		pData->bIP = true;
	}
	else {
		if (pData->cvCapture.isOpened())
			pData->cvCapture.release();
		if (pData->cvCapture.open(n) && pData->m_pVideoPlay->PrepareCamera()) {
			pData->nDeviceId = n;
			pData->szVideoFile[0] = L'\0';
		}
	}
	pData->m_csResult.Unlock();
}


int CVulpesCamera::GetDeviceVideo(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	int result = pData->nDeviceId;
	return result;
}


int CVulpesCamera::GetDeviceVideoCount(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	int result;
	result = 0;
	while (true) {
		LPCWSTR psz = GetDevNameVideoW(result);
		if (psz == NULL || psz[0] == '\0')
			break;
		result++;
	}
	return result;
}


bool CVulpesCamera::IsDeviceVideo(int n)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	return n == pData->nDeviceId;
}


bool CVulpesCamera::CanDeviceVideo(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	WORD wDriverIndex = 0;
	CHAR szName[MAX_PATH];
	INT cbName = sizeof(szName);
	CHAR szVer[MAX_PATH];
	INT cbVer = sizeof(szVer);
	result = capGetDriverDescriptionA(wDriverIndex, szName, cbName, szVer, cbVer) != FALSE;
	return result;
}


int CVulpesCamera::GetDeviceAudioCount(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	int result = waveInGetNumDevs();
	return result;
}

bool CVulpesCamera::IsDeviceAudio(int n)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = n == GetDeviceAudio();
	return result;
}

int CVulpesCamera::GetDeviceAudio(void)
{
	return 0;
}

bool CVulpesCamera::CanDeviceAudio(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bCaptureCC == false;
	return result;
}


bool CVulpesCamera::CanCmdDialog(int n)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	if (pData->nDeviceId >= 0) {
		if (n == 1) {
			result = true;
		}
		else
			result = false;
	}
	else
		result = false;

	return result;
}


void CVulpesCamera::CmdDialog(int n)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (n == 1) {
		CVulpesCameraDlg dlg(pData->pParentWnd->m_hWnd);
		dlg.DoModal();
	}
}


bool CVulpesCamera::IsVideo(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	result = pData->bMpg != FALSE;
	return result;
}


void CVulpesCamera::Stop(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;

	if (CanCmdCaptureStop())
		CmdCaptureStop();
	if (CanCmdPreviewStop())
		CmdPreviewStop();
	if (CanCmdVideoStop())
		CmdVideoStop();

	pData->bMpg = false;
	pData->bCapturing = false;
	pData->nDeviceId = -1;
	pData->bPreviewing = false;
	pData->bIP = false;

}

UINT64 CVulpesCamera::GetFrameCount(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	UINT64 result;
	if (pData->bMpg)
		result = pData->nFrameCount;
	else
		result = 0;
	return result;
}


LPCWSTR CVulpesCamera::GetDialogNameVideoW(int n)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	LPCWSTR result;
	if (pData->nDeviceId >= 0) {
		if (n == 1) {
			result = L"Video Capture Filter...";
		}
		result = L"";
	}
	else
		result = L"";
	return result;
}


LPCWSTR CVulpesCamera::GetDevNameVideoW(int n)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;

	LPCWSTR result;
	WORD wDriverIndex = n;
	TCHAR szVer[MAX_PATH];
	INT cbVer = MAX_PATH;
	if (capGetDriverDescription(wDriverIndex,
		pData->szDeviceName, sizeof(pData->szDeviceName) / sizeof(TCHAR),
		szVer, cbVer))
		result = pData->szDeviceName;
	else
		result = L"";

	CString str;
	str = result;
	if (str != L"")
	{
		if (_nCameraCnt <= n)
			_nCameraCnt = n + 1;

		return result;
	}

	WCHAR reg[MAX_PATH];
	_stprintf_s(reg, 256, L"SOFTWARE\\ELSYS\\CamIP\\%0.2u", n - _nCameraCnt);
	GetProfileStringCmd(reg, L"name", pData->szDeviceName, sizeof(pData->szDeviceName));
	return pData->szDeviceName;
}


bool CVulpesCamera::IsActive(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (pData->bCaptureCC && !IsVideo()) {
		bool rec = false;
		while (rec) {
			if (!rec)
				break;
		}
	}
	bool result = true;
	return result;
}

bool CVulpesCamera::CanPlayAVI(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bMpg == FALSE;
	return result;
}

DWORD WINAPI AVIDecorder(LPVOID lpThreadParameter);


bool CVulpesCamera::PlayVideo(LPCWSTR file, LPCWSTR fileOut, double tStart)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;
	CStringW str = file;
	INT nDot = str.ReverseFind(L'.');
	if (nDot < 0)
		result = FALSE;
	else {
		wcscpy_s(pData->szVideoFile, MAX_PATH, file);

		// Check video type
		CStringW ext = str.Mid(nDot + 1).MakeLower();
		if (ext.Compare(L"zip") == 0) {
			pData->bMpg = FALSE;
			result = false;
		}
		else {
			Stop();

			pData->nFrameCount = 0;
			pData->dblVideoDuration = 0.0;
			CStringA filea;
			filea = file;

			pData->dblStart = tStart;
			pData->m_pVideoPlay->PlayVideo();

			while (pData->dblVideoDuration <= 0.0)
				Sleep(1);

			pData->bMpg = true;
			pData->bPreviewing = true;

			result = true;
		}
	}

	return result;
}

bool CVulpesCamera::RestartVideo(double tStart)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;

	pData->dblStart = tStart;
	if (pData->bMpg)
		pData->m_pVideoPlay->StopAVI();
	if (tStart >= 0.0)
		pData->m_pVideoPlay->PlayVideo();
	result = true;

	return result;
}


double CVulpesCamera::GetVideoDuration()
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	double result;
	if (pData->bMpg)
		result = pData->dblVideoDuration;
	else
		result = 0.0;

	return result;
}

LPCWSTR CVulpesCamera::GetVideoFile()
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	LPCWSTR result;
	if (pData->bMpg) {
		if (IsVideo()) {
			result = pData->szVideoFile;
		}
		else
			result = NULL;
	}
	else
		result = NULL;

	return result;
}


void CVulpesCamera::SetCallbackData(void* data)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->pCallbackData = data;
}


void CVulpesCamera::SetCallbackOnVideoPlay(void * pFn)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->m_csResult.Lock();
	pData->pFunctionCallbackVideoPlay = (pFunctionCallbackOnPlayProcess)pFn;
	pData->m_csResult.Unlock();

}


void CVulpesCamera::SetCallbackAudio(void * pFn)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->pFunctionCallbackAudioPlay = (pFunctionCallbackOnPlayProcess)pFn;
}


void CVulpesCamera::SetNotifyWindow(HWND hWndNotify)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->m_hWndNotify = hWndNotify;
}


double CVulpesCamera::GetCurrentTimeMS(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	double result;
	result = pData->dblSampleTime;
	return result;
}

LPCWSTR CVulpesCamera::GetCurrentFile(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	LPCWSTR result;
	if (pData->bMpg)
		result = pData->m_hVideoPlayThread != NULL ? pData->szVideoFile : NULL;
	else
		result = NULL;

	return result;
}

int CVulpesCamera::GetInplaceVideo(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	int result = pData->m_nInplaceVideo;
	return result;
}


void CVulpesCamera::SetInplaceVideo(int v)
{
	//OutputDebugString(L"SetInplaceVideo\n");
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->m_nInplaceVideo = v;
}


bool CVulpesCamera::SetBPP(int bpp)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->nBPP = bpp;
	return true;
}

int CVulpesCamera::GetBPP(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	int result = pData->nBPP;
	return result;
}

void CVulpesCamera::SetDefFrameRate(double fr)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	capPreviewRate(pData->hWndCapture, (DWORD)(1000 / fr));
	pData->dblFrameRate = fr;
}

double CVulpesCamera::GetDefFrameRate(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	double result;
	result = pData->dblFrameRate;
	return result;
}


int CVulpesCamera::GetVideoType(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	int result;
	if (pData->bMpg)
		result = 5;
	else if (pData->nDeviceId >= 0)
		result = 5;
	else
		result = -1;

	return result;
}

bool CVulpesCamera::PlayIP(LPCWSTR host, int port, LPCWSTR user, LPCWSTR pwd, LPCWSTR url, float fps, bool bStop, LPCWSTR ptz, bool bRTSP)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result;

	CStringW str;
	str.Format(L"%s://%s:%u/%s", (port == 80 || port == 8080) ? L"http" : (bRTSP ? L"http" : L"rtsp"), host, port, url);

	wcscpy_s(pData->szVideoFile, MAX_PATH, str.GetBuffer());

	Stop();

	pData->nFrameCount = 0;
	pData->dblVideoDuration = 0.0;
	CStringA filea;
	filea = str;
	CvCapture *pCapture = cvCaptureFromAVI(filea);
	if (pCapture) {
		pData->nFrameCount = (INT)cvGetCaptureProperty(pCapture, CV_CAP_PROP_FRAME_COUNT);
		pData->dblVideoDuration = pData->nFrameCount / cvGetCaptureProperty(pCapture, CV_CAP_PROP_FPS);
		pData->dblFrameRate = cvGetCaptureProperty(pCapture, CV_CAP_PROP_FPS);
		cvReleaseCapture(&pCapture);
	}

	pData->dblStart = 0;
	pData->m_pVideoPlay->PlayVideo();

	pData->bMpg = true;
	pData->bPreviewing = true;
	pData->bIP = true;

	result = true;

	return result;
}

float CVulpesCamera::GetFpsIP(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	float result = pData->fpsIP;
	return result;
}

float CVulpesCamera::GetFpsIPBase(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	float result = pData->m_fBaseFps;
	return result;
}

bool CVulpesCamera::IsIP()
{

	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = pData->bIP;
	return result;
}


int CVulpesCamera::GetRotate()
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	int result;
	result = pData->nRotation;
	return result;
}

static cv::Mat rotate_frame(cv::Mat src, LPVOID pData)
{

	PIMG_PROC proc = (PIMG_PROC)pData;

	cv::Point2f srcTri[3];
	cv::Point2f dstTri[3];

	cv::Mat warp_mat(2, 3, CV_32FC1);
	cv::Mat dst;

	/// Set the dst image the same type and size as src
	dst = cv::Mat::zeros(src.cols, src.rows, src.type());


	srcTri[0] = cv::Point2f(0, 0);
	srcTri[1] = cv::Point2f(src.cols - 1.f, 0);
	srcTri[2] = cv::Point2f(0, src.rows - 1.f);

	switch (proc->value) {
	case 1:
		dstTri[0] = cv::Point2f(0, src.cols - 1.f);
		dstTri[1] = cv::Point2f(0, 0);
		dstTri[2] = cv::Point2f(src.rows - 1.f, src.cols - 1.f);
		break;
	case 2:
		dstTri[0] = cv::Point2f(src.rows - 1.f, 0);
		dstTri[1] = cv::Point2f(src.rows - 1.f, src.cols - 1.f);
		dstTri[2] = cv::Point2f(0, 0);
		break;
	default:
		return src;
	}

	/// Get the Affine Transform
	warp_mat = getAffineTransform(srcTri, dstTri);

	/// Apply the Affine Transform just found to the src image
	warpAffine(src, dst, warp_mat, cv::Size(src.rows, src.cols));
	return dst;
}


void CVulpesCamera::SetRotate(int nRotate)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->nRotation = nRotate;
	if (nRotate > 0)
		register_img_proc(rotate_frame, 0, 2, 0, nRotate);
	else
		deregister_img_proc(rotate_frame);
}


bool CVulpesCamera::CanCmdCaptureIS(void)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	bool result = IsPreviewing();
	return result;
}


double CVulpesCamera::GetAudioDelay()
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	return pData->m_dAudioDelay;
}

void CVulpesCamera::SetAudioDelay(double val)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	pData->m_dAudioDelay = val;
}


float CVulpesCamera::GetAVIPlaySpeed() const
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	return pData->m_fInputPlaySpeed;
}


void CVulpesCamera::SetVideoPlaySpeed(float speed)
{
	CCameraWndData *pData = (CCameraWndData *)m_pCapture;
	if (speed > 30.0f)
		speed = 30.0f;
	if (speed < 0.1f)
		speed = 0.1f;
	pData->m_fInputPlaySpeed = speed;
}