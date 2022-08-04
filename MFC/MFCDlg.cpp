// MFCDlg.cpp: 實作檔案
//

#include "pch.h"
#include "framework.h"
#include "MFC.h"
#include "MFCDlg.h"

#include <map>
#include <string>
#include <vector>

#include "afxdialogex.h"
#include "winioctl.h"
#include <TlHelp32.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 對 App About 使用 CAboutDlg 對話方塊

class CAboutDlg : public CDialogEx {
public:
    CAboutDlg();

    // 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV 支援

    // 程式碼實作
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCDlg 對話方塊


CMFCDlg::CMFCDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_MFC_DIALOG, pParent) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, edit);
    DDX_Control(pDX, IDC_LIST1, list_box);
    DDX_Control(pDX, IDC_EDIT2, pid_textbox);
    DDX_Control(pDX, IDC_EDIT3, set_val);
}

BEGIN_MESSAGE_MAP(CMFCDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CMFCDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CMFCDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDOK, &CMFCDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CMFCDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_BUTTON3, &CMFCDlg::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_BUTTON4, &CMFCDlg::OnBnClickedButton4)
    ON_LBN_SELCHANGE(IDC_LIST1, &CMFCDlg::OnLbnSelchangeList1)
    ON_BN_CLICKED(IDC_BUTTON193, &CMFCDlg::OnBnClickedButton193)
END_MESSAGE_MAP()


// CMFCDlg 訊息處理常式

BOOL CMFCDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    // 將 [關於...] 功能表加入系統功能表。

    // IDM_ABOUTBOX 必須在系統命令範圍之中。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr) {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
    // 框架會自動從事此作業
    SetIcon(m_hIcon, TRUE); // 設定大圖示
    SetIcon(m_hIcon, FALSE); // 設定小圖示

    // TODO: 在此加入額外的初始設定

    return TRUE; // 傳回 TRUE，除非您對控制項設定焦點
}

void CMFCDlg::OnSysCommand(UINT nID, LPARAM lParam) {
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

void CMFCDlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this); // 繪製的裝置內容

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 將圖示置中於用戶端矩形
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 描繪圖示
        dc.DrawIcon(x, y, m_hIcon);
    }
    else {
        CDialogEx::OnPaint();
    }
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
HCURSOR CMFCDlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}

std::map<std::wstring, DWORD> mp;

static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
    int length = 512;
    wchar_t* buffer = new wchar_t[length];
    DWORD process_id = 0;
    if (IsWindowVisible(hWnd)) {
        GetWindowThreadProcessId(hWnd, &process_id);
        GetWindowTextW(hWnd, buffer, length + 1);
        mp[buffer] = process_id;
    }

    delete[] buffer;
    return TRUE;
}

inline std::wstring find_process_name(DWORD search) {
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE) {
        return L"";
    }
    Process32First(processesSnapshot, &processInfo);
    if (processInfo.th32ProcessID == search) {
        CloseHandle(processesSnapshot);
        return processInfo.szExeFile;
    }
    while (Process32Next(processesSnapshot, &processInfo)) {
        if (processInfo.th32ProcessID == search) {
            CloseHandle(processesSnapshot);
            return processInfo.szExeFile;
        }
    }
    CloseHandle(processesSnapshot);
    return NULL;
}

typedef struct RTL_STRUCT {
    SIZE_T* read;
    UINT_PTR address;
    SIZE_T size;
    ULONG pid;

    enum IOMode {
        IOMODE_Write,
        IOMODE_Read,
        IOMODE_ReqBase,
        IOMODE_Unhook
    } io_mode;

    PVOID out;
    PCWSTR mod_name;
    UINT_PTR base_address;
} RtlStruct;

template <typename ... Arg>
uint64_t call_hook(const Arg ... args) {
    void* user_32 = LoadLibraryW(L"user32.dll");
    void* func_address = GetProcAddress(LoadLibraryW(L"win32u.dll"), "NtOpenCompositionSurfaceSectionInfo");
    auto func = static_cast<uint64_t(_stdcall*)(Arg ...)>(func_address);
    if (!func) {
        return 10;
    }
    return func(args ...);
}

int ProcessId;
std::wstring ModName;
ULONG64 Base;

template <typename T>
T read_mem(UINT_PTR address) {
    SIZE_T read;
    T data = {0};
    RtlStruct rtl_struct;
    rtl_struct.pid = ProcessId;
    rtl_struct.size = sizeof(T);
    rtl_struct.address = address;
    rtl_struct.read = &read;
    rtl_struct.io_mode = RTL_STRUCT::IOMODE_Read;
    rtl_struct.out = &data;
    call_hook(&rtl_struct, NULL, NULL, 0);
    return data;
}

template <typename T>
void read_mem(UINT_PTR address, T* data, int size) {
    SIZE_T read;
    RtlStruct rtl_struct;
    rtl_struct.pid = ProcessId;
    rtl_struct.size = size;
    rtl_struct.address = address;
    rtl_struct.read = &read;
    rtl_struct.io_mode = RTL_STRUCT::IOMODE_Read;
    rtl_struct.out = data;
    call_hook(NULL, &rtl_struct, NULL, NULL);
}

ULONG64 get_mod_base_address(std::wstring mod_name) {
    RtlStruct rtl_struct = {0};
    rtl_struct.pid = ProcessId;
    rtl_struct.io_mode = RTL_STRUCT::IOMODE_ReqBase;
    rtl_struct.mod_name = mod_name.c_str();
    call_hook(NULL, &rtl_struct, NULL, NULL);
    ULONG64 base = NULL;
    base = rtl_struct.base_address;
    return base;
}


void CMFCDlg::OnBnClickedButton193() {
    //建立連線
    wchar_t tmp[128];
    GetDlgItemTextW(IDC_EDIT2, tmp, 128);
    ProcessId = _wtoll(tmp);
    ModName = find_process_name(ProcessId);
    Base = get_mod_base_address(ModName);
    std::wstring data = std::to_wstring(ProcessId) + L", " + ModName + L", " + std::to_wstring(Base);
    MessageBoxW(data.c_str(), L"", MB_OK);
}

void CMFCDlg::OnBnClickedButton1() {

}


void CMFCDlg::OnBnClickedButton2() {
    //讀值
    char* buf[64];
    //int data = read_mem<int>(Base + 0xa80);//0xa80 //0x11C880
    read_mem<char*>(Base + 0x11C880, buf, 64); //0xa80 //0x11C880
    //MessageBoxW(std::to_wstring(data).c_str(), L"", MB_OK);
    MessageBoxA(NULL, (const char*)buf, "", MB_OK);
}


void CMFCDlg::OnBnClickedButton3() {
    RtlStruct rtl_struct = {0};
    rtl_struct.io_mode = RTL_STRUCT::IOMODE_Unhook;
    call_hook(NULL, &rtl_struct, NULL, NULL);
    CDialogEx::OnOK();
}


void CMFCDlg::OnBnClickedButton4() {
    LPARAM ptr = 0;
    list_box.ResetContent();
    EnumWindows(enumWindowCallback, ptr);
    for (auto& row : mp) {
        list_box.AddString((std::to_wstring(row.second) + L": " + row.first).c_str());
    }
}


void CMFCDlg::OnBnClickedOk() {
}

void CMFCDlg::OnBnClickedCancel() {
}


void CMFCDlg::OnLbnSelchangeList1() {
    // TODO: 在此加入控制項告知處理常式程式碼
}
