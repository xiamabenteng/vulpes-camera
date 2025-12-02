#include "stdafx.h"
#include "VulpesFPS.h"

CVulpesFPS::CVulpesFPS()
{
	m_dwFPSCollectSampleKeyA = 0;
	m_dwFPSCollectSampleKeyB = 0;
	memset(m_dwFPSCollectSampleValues, 0, 8192 * sizeof(DWORD64));
}


CVulpesFPS::~CVulpesFPS()
{
}


float CVulpesFPS::CalAvgFps(float fPeriod) {

	float fAvgFps = 0.f;
	DWORD64 dwFPSCollectSamplePeriod = (DWORD64)(fPeriod*1000.f);

	m_dwFPSCollectSampleKeyA = (m_dwFPSCollectSampleKeyA + 1) % 8192;

	m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyA] = GetCurrentMillisecond();

	while ((m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyA] - m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyB]) > dwFPSCollectSamplePeriod)
	{
		if ((m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyA] - m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyB]) == m_dwFPSCollectSampleKeyB) {
			break;
		}
		m_dwFPSCollectSampleKeyB = (m_dwFPSCollectSampleKeyB + 1) % 8192;
	}

	if (m_dwFPSCollectSampleKeyA == m_dwFPSCollectSampleKeyB) {
		return fAvgFps;
	}

	DWORD dwFPSCollectSampleKeyOffset = m_dwFPSCollectSampleKeyA - m_dwFPSCollectSampleKeyB;
	if (dwFPSCollectSampleKeyOffset < 0) {
		dwFPSCollectSampleKeyOffset += 8192;
	}
	m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyA] = GetCurrentMillisecond();
	DWORD64 dwFPSCollectSampleValuesDelta = m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyA] - m_dwFPSCollectSampleValues[m_dwFPSCollectSampleKeyB];
	if (dwFPSCollectSampleValuesDelta <= 0) {
		return fAvgFps;
	}
	fAvgFps = dwFPSCollectSampleKeyOffset * 1000.f / dwFPSCollectSampleValuesDelta;
	return fAvgFps;
}

void CVulpesFPS::Reset() {
	m_dwFPSCollectSampleKeyA = 0;
	m_dwFPSCollectSampleKeyB = 0;
	memset(m_dwFPSCollectSampleValues, 0, 8192 * sizeof(DWORD64));
}

DWORD64 CVulpesFPS::GetCurrentMillisecond() {
	FILETIME ft = { 0 };
	SYSTEMTIME st = { 0 };
	ULARGE_INTEGER ull = { 0 };
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft);
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return (ull.QuadPart - 116444736000000000ULL) / 10000ULL;
}