// TestPlugDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TestPlugDlg.h"


// CTestPlugDlg dialog

IMPLEMENT_DYNAMIC(CTestPlugDlg, CDialog)

CTestPlugDlg::CTestPlugDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestPlugDlg::IDD, pParent)
{

}

CTestPlugDlg::~CTestPlugDlg()
{
}

void CTestPlugDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTestPlugDlg, CDialog)
END_MESSAGE_MAP()


// CTestPlugDlg message handlers
