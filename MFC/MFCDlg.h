
// MFCDlg.h: 標頭檔
//

#pragma once


// CMFCDlg 對話方塊
class CMFCDlg : public CDialogEx
{
// 建構
public:
	CMFCDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButton3();
	CEdit edit;
	CListBox list_box;
	afx_msg void OnBnClickedButton4();
	afx_msg void OnLbnSelchangeList1();
	CEdit pid_textbox;
	CEdit set_val;
	afx_msg void OnBnClickedButton193();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
};
