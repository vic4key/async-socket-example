
// Async.Socket.Example.Dlg.h : header file
//

#pragma once

#include <vu>

// CAsyncSocketExampleDlg dialog

class CAsyncSocketExampleDlg : public CDialogEx
{
public:
	enum { IDD = IDD_ASYNC_SOCKET_EXAMPLE_DIALOG };
	CAsyncSocketExampleDlg(CWnd* pParent = nullptr);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnClose();
	afx_msg void OnBnClicked_Send();
	afx_msg void OnBnClicked_Clear();
  afx_msg void OnBnClicked_Start();
  afx_msg void OnBnClicked_Stop();
	afx_msg void OnLbnDblclk_Connections();
  afx_msg void OnBnClicked_ModeServer();
  afx_msg void OnBnClicked_ModeClient();
	DECLARE_MESSAGE_MAP()

private:
	void add_log(const std::string& text);
	void fill_connections();

private:
	DWORD m_ip;
	int m_port;
	int m_mode;
  CString  m_msg;
	CListBox m_log;
	CListBox m_connections;
	vu::AsyncSocket* m_ptr_socket;
};
