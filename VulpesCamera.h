#pragma once

#ifdef VULPESCAMERA_EXPORTS
#define VULPESCAMERA_API __declspec(dllexport)
#else
#define VULPESCAMERA_API __declspec(dllimport)
#endif

#include "VulpesFPS.h"

typedef int(*pFunctionCallbackOnPlayProcess)(void *pUserData, double dblSampleTime, BYTE *pBuffer, long lBufferSize, void *pFormat);

class CVulpesVideoPlay;

class CCameraWndData {
public:
	LPVOID pCallbackData;
	pFunctionCallbackOnPlayProcess pFunctionCallbackVideoPlay;
	pFunctionCallbackOnPlayProcess pFunctionCallbackAudioPlay;

	CVulpesFPS m_VulpesFPS;
	CCriticalSection m_csResult;
	cv::VideoCapture cvCapture;
	INT nDeviceId;
	WCHAR szDeviceName[MAX_PATH];
	WCHAR szVideoFile[MAX_PATH];
	INT nFrameCount;
	double dblVideoDuration;
	double dblStart;
	INT nBPP;
	INT nRotation;

	bool bPreviewing;

	cv::VideoWriter cvWriter;
	bool bCapturing;
	WCHAR szCaptureFile[MAX_PATH];
	bool bCapturingPaused;
	DWORD dwCaptureStarted;

	CVulpesVideoPlay  *m_pVideoPlay;
	HANDLE m_hVideoPlayThread;
	DWORD m_nVideoPlayThreadId;

	bool bMpg;

	int nUseTimeLimit;
	DWORD dwTimeLimit;

	int nUseFrameRate;
	double dblFrameRate;

	DWORD dwCaptureFileAllocSize;

	double dblSampleTime;

	float m_fPlaySpeed;
	double m_dPlaySpeedK;
	float m_fInputPlaySpeed;

	CWnd *pParentWnd;
	HWND hWndCapture;

	double m_dAudioDelay;

	float fpsIP;
	bool bIP;

	WCHAR szRegKey[MAX_PATH];

	bool bCaptureCC;

	HWND m_hWndNotify;

	int m_nInplaceVideo;

	float m_fBaseFps;

	void                *pCallbackPaint;
public:
	CCameraWndData();
};