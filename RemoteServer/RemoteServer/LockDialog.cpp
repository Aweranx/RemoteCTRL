// LockDialog.cpp : implementation file
//

#include "pch.h"
#include "RemoteServer.h"
#include "afxdialogex.h"
#include "LockDialog.h"


// CLockDialog dialog

IMPLEMENT_DYNAMIC(CLockDialog, CDialog)

CLockDialog::CLockDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_INFO, pParent)
{

}

CLockDialog::~CLockDialog()
{
}

void CLockDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLockDialog, CDialog)
END_MESSAGE_MAP()


// CLockDialog message handlers
