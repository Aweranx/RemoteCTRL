#pragma once
#include "afxdialogex.h"


// CLockDialog dialog

class CLockDialog : public CDialog
{
	DECLARE_DYNAMIC(CLockDialog)

public:
	CLockDialog(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CLockDialog();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INFO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
