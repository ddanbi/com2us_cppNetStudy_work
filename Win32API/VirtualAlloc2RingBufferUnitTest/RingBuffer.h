#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// MS Docs���� Kernel32.lib �� �ʼ� lib�ε� �Ʒ� lib�� �ʼ� lib�� -_-;;
#pragma comment(lib,"mincore.lib")

//https://docs.microsoft.com/ko-kr/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2
//int main()
//{
//	char* ringBuffer;
//	void* secondaryView;
//	unsigned int bufferSize = 0x10000;
//
//	ringBuffer = (char*)CreateRingBuffer(bufferSize, &secondaryView);
//
//	if (ringBuffer == nullptr) {
//		printf("CreateRingBuffer failed\n");
//		return 0;
//	}
//
//	//
//	// Make sure the buffer wraps properly.
//	//
//
//	ringBuffer[0] = 'a';
//
//	if (ringBuffer[bufferSize] == 'a') {
//		printf("The buffer wraps as expected\n");
//	}
//
//	UnmapViewOfFile(ringBuffer);
//	UnmapViewOfFile(secondaryView);
//
//	return 0;
//}

class RingBuffer
{
public:
	RingBuffer() = default;

	~RingBuffer() 
	{ 
		Release();
	}

	
	bool Create(const UINT64 ringBufferSize)
	{
		m_RingBufferSize = ringBufferSize;
		m_pRingBuffer = CreateRingBuffer(m_RingBufferSize, &m_pSecondaryView);

		if (m_pRingBuffer == nullptr) 
		{
			printf("CreateRingBuffer failed\n");
			return false;
		}

		Init();
		return true;
	}

	

	char* Read(const UINT32 reqReadSize)
	{
		char* pResult = nullptr;
		pResult = &m_pRingBuffer[m_ReadPos];
		m_ReadPos += reqReadSize;

		if (m_ReadPos >= m_RingBufferSize)
		{
			m_ReadPos -= m_RingBufferSize;
		}

		return pResult;		
	}

	bool Write(const UINT32 size, const char* pData)
	{
		CopyMemory(&m_pRingBuffer[m_WritePos], pData, size);
		m_WritePos += size;

		/*if (m_WritePos >= m_RingBufferSize)
		{
			m_WritePos -= m_RingBufferSize;
		}*/

		return true;
	}
			

	void UnitTest_SetWritePos(UINT64 pos) { m_WritePos = pos; }
	void UnitTest_SetReadPos(UINT64 pos) { m_ReadPos = pos; }

private:
	char* CreateRingBuffer(UINT64 bufferSize, _Outptr_ void** secondaryView)
	{
		BOOL result;
		HANDLE section = nullptr;
		SYSTEM_INFO sysInfo;
		void* ringBuffer = nullptr;
		void* placeholder1 = nullptr;
		void* placeholder2 = nullptr;
		void* view1 = nullptr;
		void* view2 = nullptr;

		GetSystemInfo(&sysInfo);

		//bufferSize�� ũ��� ������ ũ���� ��� �̾�� �Ѵ�.
		// �Ʒ� �ּ��� dwAllocationGranularity(�޸� �Ҵ� ����.���μ��� �ּ� �������� Ư�� ������ ������ �� ����ϴ� ���� ũ��. 65,536 ��)�� ����� �ƴϸ� ���з� �Ǿ� �ִ�.
		// �̰��� �Ƹ� MapViewOfFile ������ �� ������ MapViewOfFile���� ũ�Ⱑ dwAllocationGranularity�� ����̾�� �Ѵ�.
		// �׷��� �� �ڵ忡���� MapViewOfFile3 ������ ����ϱ� ������ dwAllocationGranularity ũ�Ⱑ �ƴ� ������ ũ���� ����̸� �ȴ�.				
		/*if ((bufferSize % sysInfo.dwAllocationGranularity) != 0) {
			return nullptr;
		}*/

		//
		// Reserve a placeholder region where the buffer will be mapped.
		//

		placeholder1 = (PCHAR)VirtualAlloc2(
			nullptr,
			nullptr,
			2 * bufferSize,
			MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
			PAGE_NOACCESS,
			nullptr, 0
		);

		if (placeholder1 == nullptr) {
			printf("VirtualAlloc2 failed, error %#x\n", GetLastError());
			goto Exit;
		}

		//
		// Split the placeholder region into two regions of equal size.
		//

		result = VirtualFree(
			placeholder1,
			bufferSize,
			MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER
		);

		if (result == FALSE) {
			printf("VirtualFreeEx failed, error %#x\n", GetLastError());
			goto Exit;
		}

		placeholder2 = (void*)((ULONG_PTR)placeholder1 + bufferSize);

		//
		// Create a pagefile-backed section for the buffer.
		//

		section = CreateFileMapping(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			0,
			(DWORD)bufferSize, nullptr
		);

		if (section == nullptr) {
			printf("CreateFileMapping failed, error %#x\n", GetLastError());
			goto Exit;
		}

		//
		// Map the section into the first placeholder region.
		//

		view1 = MapViewOfFile3(
			section,
			nullptr,
			placeholder1,
			0,
			bufferSize,
			MEM_REPLACE_PLACEHOLDER,
			PAGE_READWRITE,
			nullptr, 0
		);

		if (view1 == nullptr) {
			printf("MapViewOfFile3 failed, error %#x\n", GetLastError());
			goto Exit;
		}

		//
		// Ownership transferred, don��t free this now.
		//

		placeholder1 = nullptr;

		//
		// Map the section into the second placeholder region.
		//

		view2 = MapViewOfFile3(
			section,
			nullptr,
			placeholder2,
			0,
			bufferSize,
			MEM_REPLACE_PLACEHOLDER,
			PAGE_READWRITE,
			nullptr, 0
		);

		if (view2 == nullptr) {
			printf("MapViewOfFile3 failed, error %#x\n", GetLastError());
			goto Exit;
		}

		//
		// Success, return both mapped views to the caller.
		//

		ringBuffer = view1;
		*secondaryView = view2;

		placeholder2 = nullptr;
		view1 = nullptr;
		view2 = nullptr;

	Exit:

		if (section != nullptr) {
			CloseHandle(section);
		}

		if (placeholder1 != nullptr) {
			VirtualFree(placeholder1, 0, MEM_RELEASE);
		}

		if (placeholder2 != nullptr) {
			VirtualFree(placeholder2, 0, MEM_RELEASE);
		}

		if (view1 != nullptr) {
			UnmapViewOfFileEx(view1, 0);
		}

		if (view2 != nullptr) {
			UnmapViewOfFileEx(view2, 0);
		}

		return (char*)ringBuffer;
	}
	
	void Init()
	{
		m_WritePos = 0;
		m_ReadPos = 0;
	}

	void Release()
	{
		UnmapViewOfFile(m_pRingBuffer);
		UnmapViewOfFile(m_pSecondaryView);
	}

	
	UINT64 m_RingBufferSize = 0;
	
	char* m_pRingBuffer = nullptr;
	void* m_pSecondaryView = nullptr;
	UINT64 m_WritePos = 0;
	UINT64 m_ReadPos = 0;
	
};
