#pragma once
#include <iostream>
#include <windows.h>
#include <vector>
#include <atlimage.h>
#include <fstream>

// � ������ ����� ����������� �������, ������� ������ �������� ������.
// ��� ���� ��������: �������� ������ main � �������.
// � �������� ���������� ��������� �����, ���� �������� ��������.

// TODO: 
// ������� ��������� ������� CustomMsgTypes::GetScreenShot
// �������� ������ � ������������ ��������������� ����� � headers/helpers - 
// ������� �������� ��������� ���� CustomMsgTypes::GetScreenShot � server.cpp / client.cpp
// �������� ���������� <atlimage.h> � net_common.h 
// ��������� � ����������� �������� ��������� ���������
// ��� �������� �������� �� ������, � ����� server.cpp ����������� �������, 
//          ������� ����� ���������� ���� ��������� � ������� ���� ��������, 
//          ������� � �������� ��������� �������

void DoScreen(std::string source) {
    using namespace std;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemDC, hBitmap);


    BitBlt(hdcMemDC, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

    SelectObject(hdcMemDC, hOldBitmap);
    DeleteDC(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);

    std::vector<BYTE> buf;
    IStream* stream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    CImage image;
    image.Attach(hBitmap);
    image.Save(stream, Gdiplus::ImageFormatPNG);

    ULARGE_INTEGER liSize;
    IStream_Size(stream, &liSize);
    DWORD len = liSize.LowPart;

    IStream_Reset(stream);
    buf.resize(len);
    IStream_Read(stream, &buf[16], len);

    stream->Release();

    std::fstream fi;
    fi.open(source, std::fstream::binary | std::fstream::out);
    fi.write(reinterpret_cast<const char*>(&buf[16]), buf.size() * sizeof(BYTE));
    fi.close();

    DeleteObject(hBitmap);
}
