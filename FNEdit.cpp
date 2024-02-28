#define _FAR_NO_NAMELESS_UNIONS
#define _FAR_USE_FARFINDDATA
#include "plugin.hpp"
#include "farkeys.hpp"

static struct PluginStartupInfo Info;
static char PluginRootKey[80];
extern "C"
{
  void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info);
  HANDLE WINAPI _export OpenPlugin(int OpenFrom,int Item);
  void WINAPI _export GetPluginInfo(struct PluginInfo *Info);
  int WINAPI _export Configure(int ada);
};

static struct Options 
{
  int cexit, showmod, showpath;
} Opt;

enum EditLng 
{
  MNoLngStringDefined = -1,
  MOK,
  MCancel,
  MTitle,
  MNoFile,
  MFilePanReq,
  MExitConf,
  MCExit,
  MShowmod,
  MShowpath
};

static const char *GetMsg(enum EditLng MsgId) 
{
  return Info.GetMsg(Info.ModuleNumber, MsgId);
}

void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info)
{
  HKEY hKey;
  DWORD dw, dwSize = sizeof(DWORD);
  ::Info=*Info;
  Opt.cexit = 1;
  Opt.showmod = 0;
  Opt.showpath = 0;
  strcpy(PluginRootKey, Info->RootKey);
  strcat(PluginRootKey, "\\FileNameEdit");
  if (RegOpenKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
  {
    if (RegQueryValueEx(hKey, "cexit", NULL, NULL, (unsigned char *)&dw, &dwSize) == ERROR_SUCCESS)
      Opt.cexit = dw;
    if (RegQueryValueEx(hKey, "showmod", NULL, NULL, (unsigned char *)&dw, &dwSize) == ERROR_SUCCESS)
      Opt.showmod = dw;
    if (RegQueryValueEx(hKey, "showpath", NULL, NULL, (unsigned char *)&dw, &dwSize) == ERROR_SUCCESS)
      Opt.showpath = dw;
    RegCloseKey(hKey);
  }
}

void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
  Info->StructSize = sizeof(*Info);
  Info->Flags = 0;
  Info->DiskMenuStringsNumber = 0;
  static const char *PluginMenuStrings[] = {GetMsg(MTitle)};
  Info->PluginMenuStrings = PluginMenuStrings;
  Info->PluginConfigStrings = PluginMenuStrings;
  Info->PluginMenuStringsNumber = 1;
  Info->PluginConfigStringsNumber = 1;
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom, int Item)
{
  struct PanelInfo AInfo;
  struct EditorInfo EI;
  struct EditorSetParameter rt;
  struct EditorGetString egs;
  char sd[3];
  char sm[3];
  char sy[5];
  char stitle[256];
  char * s = NULL;
  char * fn = NULL;
  char nn[1024];
  int i, k, KeyCode, curLine = -2;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  SYSTEMTIME ft;
  FILETIME nt;
  BOOL Done = FALSE;
  INPUT_RECORD rec; 

  if (!Info.Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &AInfo))
    return INVALID_HANDLE_VALUE;
  if (AInfo.PanelType != PTYPE_FILEPANEL) 
  {
    const char *MsgItems[] = {GetMsg(MTitle), GetMsg(MFilePanReq), GetMsg(MOK)};
    Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, sizeof(MsgItems) / sizeof(MsgItems[0]), 1);
    return INVALID_HANDLE_VALUE;
  }

  Info.Editor("-", GetMsg(MNoFile), 0, 0, -1, -1, EF_IMMEDIATERETURN + EF_NONMODAL + EF_CREATENEW, 0, 1);

  rt.Type = ESPT_CHARTABLE;
  rt.Param.iParam = 1;
  Info.EditorControl(ECTL_SETPARAM, &rt);  // установка кодовой страницы дос

  // заполнение редактора именами файлов
  for (i = 0; i < AInfo.SelectedItemsNumber; i++)
  {
    fn = AInfo.SelectedItems[i].FindData.cFileName;
    if (!Opt.showpath)
    {
      s = strrchr(fn, '\\');
      // если в строке есть символ "\", то берем строку начинающуюся после последнего символа "\" - файл без пути
      // или просто берем имя файла - если он без пути
      if (s)  
        Info.EditorControl(ECTL_INSERTTEXT, ++s);
      else  
        Info.EditorControl(ECTL_INSERTTEXT, fn);
    }
    else  
        Info.EditorControl(ECTL_INSERTTEXT, fn);
    Info.EditorControl(ECTL_INSERTSTRING, NULL);
  }
  // удаляет последнюю пустую строчку и переводит курсор в начало редактора
  Info.EditorControl(ECTL_PROCESSKEY, (void *)KEY_BS);
  Info.EditorControl(ECTL_PROCESSKEY, (void *)(KEY_CTRL + KEY_HOME)); 
  Info.EditorControl(ECTL_REDRAW, NULL);
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  while (!Done)
  {
    Info.EditorControl(ECTL_READINPUT, &rec);
    if ((rec.EventType == KEY_EVENT) && rec.Event.KeyEvent.bKeyDown)
    {
      KeyCode = rec.Event.KeyEvent.wVirtualKeyCode;
      if ((KeyCode == VK_ESCAPE) || (KeyCode == VK_F10))
      {
        if (Opt.cexit)
        {
          const char *MsgItems[] = {GetMsg(MTitle), GetMsg(MExitConf), GetMsg(MOK), GetMsg(MCancel)};
          k = Info.Message(Info.ModuleNumber, FMSG_WARNING, NULL, MsgItems, sizeof(MsgItems) / sizeof(MsgItems[0]), 2);
          if (k == 0)
          {
            Done = TRUE;
            Info.EditorControl(ECTL_QUIT, NULL);
          }
        }
        else
        {
          Done = TRUE;
          Info.EditorControl(ECTL_QUIT, NULL);
        }
      }
      else
        if ((KeyCode == VK_F2) && ((rec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) == 0))
        {          
          i = 0;
          while (i < AInfo.SelectedItemsNumber)
          {
            egs.StringNumber = i;
            Info.EditorControl(ECTL_GETSTRING, &egs);
            fn = AInfo.SelectedItems[i].FindData.cFileName;
            if (!Opt.showpath)
            {
              s = strrchr(fn, '\\');
              if (s)
              {
                strncpy(nn, fn, s - fn + 1);
                nn[s - fn + 1] = '\0';
                strcat(nn, egs.StringText);
              }
              else  
                strcpy(nn, egs.StringText);
            }
            else  
              strcpy(nn, egs.StringText);
            if (MoveFileEx(fn, nn, NULL) == 0)
            {
              const char *MsgItems[] = {GetMsg(MTitle), egs.StringText};
              k = Info.Message(Info.ModuleNumber, FMSG_WARNING + FMSG_ERRORTYPE + 
                FMSG_MB_ABORTRETRYIGNORE, NULL, MsgItems, 
                sizeof(MsgItems) / sizeof(MsgItems[0]), 0);
              if (k == 0)
                i = AInfo.SelectedItemsNumber;
              else
                if ((k == 2) || (k == -1))
                  i++;
            }
            else
              i++;
          }         
          Done = TRUE;
          Info.EditorControl(ECTL_QUIT, NULL);
        }
        else
          Info.EditorControl(ECTL_PROCESSINPUT, &rec);
    }
    else
      if ((rec.EventType == MOUSE_EVENT) && 
        (rec.Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) && 
        (rec.Event.MouseEvent.dwMousePosition.Y == (csbi.dwSize.Y - 1)))
        ;
        else
          Info.EditorControl(ECTL_PROCESSINPUT, &rec);

    // отрисовка в заголовке имени файла и даты модификации (опция), если курсор перешел на другую строку
    Info.EditorControl(ECTL_GETINFO, &EI);
    if (EI.CurLine != curLine)
    {
      curLine = EI.CurLine;
      if (EI.CurLine > (AInfo.SelectedItemsNumber - 1))
        Info.EditorControl(ECTL_SETTITLE, (void *)GetMsg(MNoFile));
      else
      {
        strcpy(stitle, AInfo.SelectedItems[curLine].FindData.cFileName);
        if (Opt.showmod)
        {
          FileTimeToLocalFileTime(&(AInfo.SelectedItems[curLine].FindData.ftLastWriteTime), &nt);
          FileTimeToSystemTime(&nt, &ft);
          itoa(ft.wDay, sd, 10);
          if (sd[1] == '\0')
          {
            sd[1] = sd[0];
            sd[0] = '0';
            sd[2] = '\0';
          }
          itoa(ft.wMonth, sm, 10);
          if (sm[1] == '\0')
          {
            sm[1] = sm[0];
            sm[0] = '0';
            sm[2] = '\0';
          }
          itoa(ft.wYear, sy, 10);
          sy[0] = sy[2];
          sy[1] = sy[3];
          sy[2] = '\0';
          strcat(stitle, " ");
          strcat(stitle, sd);
          strcat(stitle, ".");
          strcat(stitle, sm);
          strcat(stitle, ".");
          strcat(stitle, sy);
        }
        Info.EditorControl(ECTL_SETTITLE, stitle);
      }
    }
  }
  return INVALID_HANDLE_VALUE;
}

int WINAPI _export Configure( int ada )
{
  static struct InitDialogItem 
  {
    unsigned char Type;
    unsigned char X1, Y1, X2, Y2;
    enum EditLng Data;
    unsigned int Flags;
  } InitItems[] = 
  {
    /*  0 */ { DI_DOUBLEBOX, 2,   1,   44, 11, MTitle,   0},               
    /*  1 */ { DI_BUTTON,    0,   10,  0,  0,  MOK,      DIF_CENTERGROUP}, 
    /*  2 */ { DI_BUTTON,    0,   10,  0,  0,  MCancel,  DIF_CENTERGROUP}, 
    /*  3 */ { DI_CHECKBOX,  4,   3,   0,  0,  MCExit,   0},
    /*  4 */ { DI_CHECKBOX,  4,   5,   0,  0,  MShowmod, 0},
    /*  5 */ { DI_CHECKBOX,  4,   7,   0,  0,  MShowpath, 0},
  };
  struct FarDialogItem DialogItems[sizeof(InitItems) / sizeof(InitItems[0])];
  
  int i;
  for (i = sizeof(InitItems) / sizeof(InitItems[0]) - 1; i >= 0; i--) 
  {
    DialogItems[i].Type  = InitItems[i].Type;
    DialogItems[i].X1    = InitItems[i].X1;
    DialogItems[i].Y1    = InitItems[i].Y1;
    DialogItems[i].X2    = InitItems[i].X2;
    DialogItems[i].Y2    = InitItems[i].Y2;
    DialogItems[i].Focus = FALSE;
    DialogItems[i].Flags = InitItems[i].Flags;
    strcpy(DialogItems[i].Data.Data, GetMsg(InitItems[i].Data));
  }
  DialogItems[3].Param.Selected = Opt.cexit;
  DialogItems[4].Param.Selected = Opt.showmod;
  DialogItems[5].Param.Selected = Opt.showpath;
  int ExitCode = Info.Dialog(Info.ModuleNumber, -1, -1, 47, 13, "Config", DialogItems,(sizeof(DialogItems)/sizeof(DialogItems[0])));
  if (ExitCode == 1)
  {
    Opt.cexit = DialogItems[3].Param.Selected;
    Opt.showmod = DialogItems[4].Param.Selected;
    Opt.showpath = DialogItems[5].Param.Selected;
    HKEY hKey;
    DWORD dw, dw1, dwSize = sizeof(DWORD);
    
    if (RegCreateKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dw1) == ERROR_SUCCESS)
    {
      dw = (DWORD)Opt.cexit;
      RegSetValueEx(hKey, "cexit", 0, REG_DWORD, (BYTE *)&dw, dwSize);
      dw = (DWORD)Opt.showmod;
      RegSetValueEx(hKey, "showmod", 0, REG_DWORD, (BYTE *)&dw, dwSize);
      dw = (DWORD)Opt.showpath;
      RegSetValueEx(hKey, "showpath", 0, REG_DWORD, (BYTE *)&dw, dwSize);
      RegCloseKey(hKey);
    }
  }
  return true;
}