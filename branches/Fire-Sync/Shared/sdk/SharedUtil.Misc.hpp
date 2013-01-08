/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        SharedUtil.Misc.hpp
*  PURPOSE:
*  DEVELOPERS:  ccw <chris@codewave.co.uk>
*               Alberto Alonso <rydencillo@gmail.com>
*               Cecill Etheredge <ijsf@gmx-topmail.de>
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include "UTF8.h"
#ifdef WIN32
    #include <direct.h>
    #include <shellapi.h>
#endif

#ifdef WIN32

#define TROUBLE_URL1 "http://updatesa.multitheftauto.com/sa/trouble/?v=%VERSION%&id=%ID%&tr=%TROUBLE%"

//
// Get startup directory as saved in the registry by the launcher
// Used in the Win32 Client only
//
SString SharedUtil::GetMTASABaseDir()
{
    static TCHAR szInstallRoot[MAX_PATH]=TEXT("");
    if( !szInstallRoot[0] )
    {
        memset ( szInstallRoot, 0, MAX_PATH );

        HKEY hkey = NULL;
        DWORD dwBufferSize = MAX_PATH;
        DWORD dwType = 0;
        if ( RegOpenKeyEx ( HKEY_CURRENT_USER, "Software\\Multi Theft Auto: San Andreas", 0, KEY_READ, &hkey ) == ERROR_SUCCESS )
        {
            // Read out the MTA installpath
            if ( RegQueryValueEx ( hkey, "Last Run Location", NULL, &dwType, (LPBYTE)szInstallRoot, &dwBufferSize ) != ERROR_SUCCESS ||
                strlen ( szInstallRoot ) == 0 )
            {
                MessageBox ( 0, "Multi Theft Auto has not been installed properly, please reinstall.", "Error", MB_OK );
                RegCloseKey ( hkey );
                TerminateProcess ( GetCurrentProcess (), 9 );
            }
            RegCloseKey ( hkey );
        }
    }
    return szInstallRoot;
}

//
// Turns a relative MTASA path i.e. "MTA\file.dat"
// into an absolute MTASA path i.e. "C:\Program Files\MTA San Andreas\MTA\file.dat"
//
SString SharedUtil::CalcMTASAPath ( const SString& strPath )
{
    SString strNewPath = GetMTASABaseDir();
    strNewPath += '\\';
    strNewPath += strPath;
    return strNewPath;
}


//
// Write a registry string value
//
static void WriteRegistryStringValue ( HKEY hkRoot, LPCSTR szSubKey, LPCSTR szValue, const SString& strBuffer )
{
    HKEY hkTemp;
    RegCreateKeyEx ( hkRoot, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkTemp, NULL );
    if ( hkTemp )
    {
        RegSetValueEx ( hkTemp, szValue, NULL, REG_SZ, (LPBYTE)strBuffer.c_str (), strBuffer.length () + 1 );
        RegCloseKey ( hkTemp );
    }
}

//
// Read a registry string value
//
static SString ReadRegistryStringValue ( HKEY hkRoot, LPCSTR szSubKey, LPCSTR szValue, int* iResult )
{
    // Clear output
    SString strOutResult = "";

    bool bResult = false;
    HKEY hkTemp = NULL;
    if ( RegOpenKeyEx ( hkRoot, szSubKey, 0, KEY_READ, &hkTemp ) == ERROR_SUCCESS ) 
    {
        DWORD dwBufferSize;
        if ( RegQueryValueExA ( hkTemp, szValue, NULL, NULL, NULL, &dwBufferSize ) == ERROR_SUCCESS )
        {
            char *szBuffer = static_cast < char* > ( alloca ( dwBufferSize + 1 ) );
            if ( RegQueryValueExA ( hkTemp, szValue, NULL, NULL, (LPBYTE)szBuffer, &dwBufferSize ) == ERROR_SUCCESS )
            {
                strOutResult = szBuffer;
                bResult = true;
            }
        }
        RegCloseKey ( hkTemp );
    }
    if ( iResult )
        *iResult = bResult;
    return strOutResult;
}


//
// Delete a registry key and its subkeys
//
static bool DeleteRegistryKey ( HKEY hkRoot, LPCSTR szSubKey )
{
    return RegDeleteKey ( hkRoot, szSubKey ) == ERROR_SUCCESS;
}



//
// Run ShellExecute with these parameters after exit
//
void SharedUtil::SetOnQuitCommand ( const SString& strOperation, const SString& strFile, const SString& strParameters, const SString& strDirectory, const SString& strShowCmd )
{
    // Encode into a string and set a registry key
    SString strValue ( "%s\t%s\t%s\t%s\t%s", strOperation.c_str (), strFile.c_str (), strParameters.c_str (), strDirectory.c_str (), strShowCmd.c_str () );
    WriteRegistryStringValue ( HKEY_CURRENT_USER, "Software\\Multi Theft Auto: San Andreas", "OnQuitCommand", strValue );
}


#ifdef MTASA_VERSION_MAJOR
//
// What to do on next restart
//
void SharedUtil::SetOnRestartCommand ( const SString& strOperation, const SString& strFile, const SString& strParameters, const SString& strDirectory, const SString& strShowCmd )
{
    // Encode into a string and set a registry key
    SString strVersion ( "%d.%d.%d-%d.%05d" ,MTASA_VERSION_MAJOR ,MTASA_VERSION_MINOR ,MTASA_VERSION_MAINTENANCE ,MTASA_VERSION_TYPE ,MTASA_VERSION_BUILD );
    SString strValue ( "%s\t%s\t%s\t%s\t%s\t%s", strOperation.c_str (), strFile.c_str (), strParameters.c_str (), strDirectory.c_str (), strShowCmd.c_str (), strVersion.c_str () );
    WriteRegistryStringValue ( HKEY_CURRENT_USER, "Software\\Multi Theft Auto: San Andreas", "OnRestartCommand", strValue );
}


//
// What to do on next restart
//
bool SharedUtil::GetOnRestartCommand ( SString& strOperation, SString& strFile, SString& strParameters, SString& strDirectory, SString& strShowCmd )
{
    SString strOnRestartCommand = ReadRegistryStringValue ( HKEY_CURRENT_USER, "Software\\Multi Theft Auto: San Andreas", "OnRestartCommand", NULL );
    SetOnRestartCommand ( "" );

    std::vector < SString > vecParts;
    strOnRestartCommand.Split ( "\t", vecParts );
    if ( vecParts.size () > 5 && vecParts[0].length () )
    {
        SString strVersion ( "%d.%d.%d-%d.%05d", MTASA_VERSION_MAJOR, MTASA_VERSION_MINOR, MTASA_VERSION_MAINTENANCE, MTASA_VERSION_TYPE, MTASA_VERSION_BUILD );
        if ( vecParts[5] == strVersion )
        {
            strOperation = vecParts[0];
            strFile = vecParts[1];
            strParameters = vecParts[2];
            strDirectory = vecParts[3];
            strShowCmd = vecParts[4];
            return true;
        }
        AddReportLog( 4000, SString ( "OnRestartCommand disregarded due to version change %s -> %s", vecParts[5].c_str (), strVersion.c_str () ) );
    }
    return false;
}
#endif


//
// Application settings
// 

//
// Get/set string
//
void SharedUtil::SetApplicationSetting ( const SString& strPath, const SString& strKey, const SString& strValue )
{
    SString strRegFullPath  = SStringX( "Software\\Multi Theft Auto: San Andreas\\Settings\\" ) + strPath.Replace( ".", "\\" );
    WriteRegistryStringValue ( HKEY_CURRENT_USER, strRegFullPath, strKey, strValue );
}

SString SharedUtil::GetApplicationSetting ( const SString& strPath, const SString& strKey )
{
    SString strRegFullPath  = SStringX( "Software\\Multi Theft Auto: San Andreas\\Settings\\" ) + strPath.Replace( ".", "\\" );
    return ReadRegistryStringValue ( HKEY_CURRENT_USER, strRegFullPath, strKey, NULL );
}

//
// Get/set int
//
void SharedUtil::SetApplicationSettingInt ( const SString& strPath, const SString& strKey, int iValue )
{
    SetApplicationSetting ( strPath, strKey, SString ( "%d", iValue ) );
}

int SharedUtil::GetApplicationSettingInt ( const SString& strPath, const SString& strKey )
{
    return atoi ( GetApplicationSetting ( strPath, strKey ) );
}

//
// Get/set string - Combined path and key i.e. "group.key"
//
SString SharedUtil::GetApplicationSetting ( const SString& strPathKey )
{
    SString strPath;
    SString strKey = strPathKey.SplitRight ( ".", &strPath, -1 );
    if ( !strPath.length () )
        strPath = "general";
    return GetApplicationSetting ( strPath, strKey );
}

void SharedUtil::SetApplicationSetting ( const SString& strPathKey, const SString& strValue )
{
    SString strPath;
    SString strKey = strPathKey.SplitRight ( ".", &strPath, -1 );
    if ( !strPath.length () )
        strPath = "general";
    SetApplicationSetting ( strPath, strKey, strValue );
}

//
// Get/set int - Combined path and key i.e. "group.key"
//
void SharedUtil::SetApplicationSettingInt ( const SString& strPathKey, int iValue )
{
    SetApplicationSetting ( strPathKey, SString ( "%d", iValue ) );
}

int SharedUtil::GetApplicationSettingInt ( const SString& strPathKey )
{
    return atoi ( GetApplicationSetting ( strPathKey ) );
}

// Delete a setting key
static bool DeleteApplicationSettingKey ( const SString& strPathKey )
{
    SString strRegFullPath  = SStringX( "Software\\Multi Theft Auto: San Andreas\\Settings\\" ) + strPathKey.Replace( ".", "\\" );
    return DeleteRegistryKey ( HKEY_CURRENT_USER, strRegFullPath );
}



//
// WatchDog
//

void SharedUtil::WatchDogReset ( void )
{
    DeleteApplicationSettingKey ( "watchdog" );
}

// Section
bool SharedUtil::WatchDogIsSectionOpen ( const SString& str )
{
    return GetApplicationSettingInt ( "watchdog", str ) != 0;
}

void SharedUtil::WatchDogBeginSection ( const SString& str )
{
    SetApplicationSettingInt ( "watchdog", str, 1 );
}

void SharedUtil::WatchDogCompletedSection ( const SString& str )
{
    SetApplicationSettingInt ( "watchdog", str, 0 );
}

// Counter
void SharedUtil::WatchDogIncCounter ( const SString& str )
{
    SetApplicationSettingInt ( "watchdog", str, WatchDogGetCounter ( str ) + 1 );
}

int SharedUtil::WatchDogGetCounter ( const SString& str )
{
    return GetApplicationSettingInt ( "watchdog", str );
}

void SharedUtil::WatchDogClearCounter ( const SString& str )
{
    SetApplicationSettingInt ( "watchdog", str, 0 );
}


bool SharedUtil::WatchDogWasUncleanStop ( void )
{
    return GetApplicationSettingInt ( "watchdog", "uncleanstop" ) != 0;
}

void SharedUtil::WatchDogSetUncleanStop ( bool bOn )
{
    SetApplicationSettingInt ( "watchdog", "uncleanstop", bOn );
}


//
// Direct players to info about trouble
//
void SharedUtil::BrowseToSolution ( const SString& strType, bool bAskQuestion, bool bTerminateProcess )
{
    AddReportLog ( 3200, SString ( "Trouble %s", *strType ) );

    if ( !bAskQuestion || IDYES == MessageBox( 0, "Do you want to see some on-line help about this problem ?", "MTA: San Andreas", MB_ICONQUESTION|MB_YESNO ) )
    {
        SString strQueryURL = GetApplicationSetting ( "trouble-url" );
        if ( strQueryURL == "" )
            strQueryURL = TROUBLE_URL1;
        strQueryURL = strQueryURL.Replace ( "%VERSION%", GetApplicationSetting ( "mta-version-ext" ) );
        strQueryURL = strQueryURL.Replace ( "%ID%", GetApplicationSetting ( "serial" ) );
        strQueryURL = strQueryURL.Replace ( "%TROUBLE%", strType );
        ShellExecute ( NULL, "open", strQueryURL.c_str (), NULL, NULL, SW_SHOWNORMAL );
    }

    if ( bTerminateProcess )
        TerminateProcess ( GetCurrentProcess (), 1 );
}


//
// For tracking results of new features
//
static SString GetReportLogHeaderText ( void )
{
    SString strMTABuild      = GetApplicationSetting ( "mta-version-ext" ).SplitRight ( "-" );
    SString strOSVersion     = GetApplicationSetting ( "os-version" );
    SString strRealOSVersion = GetApplicationSetting ( "real-os-version" );
    SString strIsAdmin       = GetApplicationSetting ( "is-admin" );

    SString strResult = "[";
    if ( strMTABuild.length () )
        strResult += strMTABuild + " ";
    if ( strOSVersion.length () )
    {
        if ( strOSVersion == strRealOSVersion )
            strResult += strOSVersion + " ";
        else
            strResult += strOSVersion + "(" + strRealOSVersion + ") ";
    }
    if ( strIsAdmin == "1" )
        strResult += "a";

    return strResult.TrimEnd ( " " ) + "]";
}


void SharedUtil::AddReportLog ( uint uiId, const SString& strText )
{
    SString strPathFilename = PathJoin ( GetMTALocalAppDataPath (), "report.log" );
    MakeSureDirExists ( strPathFilename );

    SString strMessage ( "%u: %s %s - %s\n", uiId, GetTimeString ( true, false ).c_str (), GetReportLogHeaderText ().c_str (), strText.c_str () );
    FileAppend ( strPathFilename, &strMessage.at ( 0 ), strMessage.length () );
#if MTA_DEBUG
    OutputDebugString ( SStringX ( "ReportLog: " ) + strMessage );
#endif
}

void SharedUtil::SetReportLogContents ( const SString& strText )
{
    SString strPathFilename = PathJoin ( GetMTALocalAppDataPath (), "report.log" );
    MakeSureDirExists ( strPathFilename );
    FileSave ( strPathFilename, strText.length () ? &strText.at ( 0 ) : NULL, strText.length () );
}

SString SharedUtil::GetReportLogContents ( void )
{
    SString strReportFilename = PathJoin ( GetMTALocalAppDataPath (), "report.log" );
    // Load file into a string
    std::vector < char > buffer;
    FileLoad ( strReportFilename, buffer );
    buffer.push_back ( 0 );
    return &buffer[0];
}
#endif


static char ToHexChar ( char c )
{
    return c > 9 ? c - 10 + 'A' : c + '0';
}

static char FromHexChar ( char c )
{
    return c > '9' ? c - 'A' + 10 : c - '0';
}

SString SharedUtil::EscapeString ( const SString& strText, const SString& strDisallowedChars, char cSpecialChar )
{
    // Replace each disallowed char with #FF
    SString strResult;
    for ( uint i = 0 ; i < strText.length () ; i++ )
    {
        char c = strText[i];
        if ( strDisallowedChars.find ( c ) == std::string::npos && c != cSpecialChar )
            strResult += c;
        else
        {
            strResult += cSpecialChar;
            strResult += ToHexChar ( c >> 4 );
            strResult += ToHexChar ( c & 0x0f );
        }
    }
    return strResult;
}


SString SharedUtil::UnescapeString ( const SString& strText, char cSpecialChar )
{
    SString strResult;
    // Replace #FF with char
    for ( uint i = 0 ; i < strText.length () ; i++ )
    {
        char c = strText[i];
        if ( c == cSpecialChar && i < strText.length () - 2 )
        {
            char c0 = FromHexChar ( strText[++i] );
            char c1 = FromHexChar ( strText[++i] );
            c = ( c0 << 4 ) | c1;
        }
        strResult += c;
    }
    return strResult;
}

#ifdef ExpandEnvironmentStringsForUser
//
// eg "%HOMEDRIVE%" -> "C:"
//
SString SharedUtil::ExpandEnvString ( const SString& strInput )
{
    HANDLE hProcessToken;
    if ( !OpenProcessToken ( GetCurrentProcess (), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE, &hProcessToken ) )
        return strInput;

    const static int iBufferSize = 32000;
    char envBuf [ iBufferSize + 2 ];
    ExpandEnvironmentStringsForUser ( hProcessToken, strInput, envBuf, iBufferSize );
    return envBuf;
}
#endif


//
// Cross platform critical section
//
#ifdef WIN32

    SharedUtil::CCriticalSection::CCriticalSection ( void )
    {
        m_pCriticalSection = new CRITICAL_SECTION;
        InitializeCriticalSection ( ( CRITICAL_SECTION* ) m_pCriticalSection );
    }

    SharedUtil::CCriticalSection::~CCriticalSection ( void )
    {
        DeleteCriticalSection ( ( CRITICAL_SECTION* ) m_pCriticalSection );
        delete ( CRITICAL_SECTION* ) m_pCriticalSection;
    }

    void SharedUtil::CCriticalSection::Lock ( void )
    {
        if ( m_pCriticalSection )
            EnterCriticalSection ( ( CRITICAL_SECTION* ) m_pCriticalSection );
    }

    void SharedUtil::CCriticalSection::Unlock ( void )
    {
        if ( m_pCriticalSection )
            LeaveCriticalSection ( ( CRITICAL_SECTION* ) m_pCriticalSection );
    }

#else
    #include <pthread.h>

    SharedUtil::CCriticalSection::CCriticalSection ( void )
    {
        m_pCriticalSection = new pthread_mutex_t;
        pthread_mutex_init ( ( pthread_mutex_t* ) m_pCriticalSection, NULL );
    }

    SharedUtil::CCriticalSection::~CCriticalSection ( void )
    {
        pthread_mutex_destroy ( ( pthread_mutex_t* ) m_pCriticalSection );
        delete ( pthread_mutex_t* ) m_pCriticalSection;
    }

    void SharedUtil::CCriticalSection::Lock ( void )
    {
        pthread_mutex_lock ( ( pthread_mutex_t* ) m_pCriticalSection );
    }

    void SharedUtil::CCriticalSection::Unlock ( void )
    {
        pthread_mutex_unlock ( ( pthread_mutex_t* ) m_pCriticalSection );
    }

#endif



//
// Expiry stuff
//
#ifdef WIN32
    #include <time.h>

    #define YEAR ((((__DATE__ [7]-'0')*10+(__DATE__ [8]-'0'))*10+(__DATE__ [9]-'0'))*10+(__DATE__ [10]-'0'))

    /* Month: 0 - 11 */
    #define MONTH (__DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 0 : 5) \
                  : __DATE__ [2] == 'b' ? 1 \
                  : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M'? 2 : 3) \
                  : __DATE__ [2] == 'y' ? 4 \
                  : __DATE__ [2] == 'l' ? 6 \
                  : __DATE__ [2] == 'g' ? 7 \
                  : __DATE__ [2] == 'p' ? 8 \
                  : __DATE__ [2] == 't' ? 9 \
                  : __DATE__ [2] == 'v' ? 10 : 11)

    #define DAY ((__DATE__ [4]==' ' ? 0 : __DATE__[4]-'0')*10+(__DATE__[5]-'0'))

    int SharedUtil::GetBuildAge ( void )
    {
        tm when;
        memset ( &when, 0, sizeof ( when ) );
        when.tm_year = YEAR - 1900;
        when.tm_mon = MONTH;
        when.tm_mday = DAY;
        return ( int )( time ( NULL ) - mktime( &when ) ) / ( 60 * 60 * 24 );
    }

#if defined(MTA_DM_EXPIRE_DAYS)
    int SharedUtil::GetDaysUntilExpire ( void )
    {
        tm when;
        memset ( &when, 0, sizeof ( when ) );
        when.tm_year = YEAR - 1900;
        when.tm_mon = MONTH;
        when.tm_mday = DAY + MTA_DM_EXPIRE_DAYS;
        return ( int )( mktime( &when ) - time ( NULL ) ) / ( 60 * 60 * 24 );
    }

#endif
#endif


// Copied from CChatLine::RemoveColorCode()
std::string SharedUtil::RemoveColorCode ( const char* szString )
{
    std::string strOut;
    const char* szStart = szString;
    const char* szEnd = szString;

    while ( true )
    {
        if ( *szEnd == '\0' )
        {
            strOut.append ( szStart, szEnd - szStart );
            break;
        }
        else
        {
            bool bIsColorCode = false;
            if ( *szEnd == '#' )
            {
                bIsColorCode = true;
                for ( int i = 0; i < 6; i++ )
                {
                    char c = szEnd [ 1 + i ];
                    if ( !isdigit ( (unsigned char)c ) && (c < 'A' || c > 'F') && (c < 'a' || c > 'f') )
                    {
                        bIsColorCode = false;
                        break;
                    }
                }
            }

            if ( bIsColorCode )
            {
                strOut.append ( szStart, szEnd - szStart );
                szStart = szEnd + 7;
                szEnd = szStart;
            }
            else
            {
                szEnd++;
            }
        }
    }

    return strOut;
}


// Convert a standard std::string into a UTF-8 std::wstring
std::wstring SharedUtil::ConvertToUTF8 (const std::string& input)
{
    return utf8_mbstowcs (input);
}

// Convert a std::wstring into an ANSI encoded string
std::string SharedUtil::ConvertToANSI (const std::wstring& input)
{
    return utf8_wcstombs (input);
}


#ifdef MTA_DEBUG
//
// Output timestamped line into the debugger
//
void SharedUtil::OutputDebugLine ( const char* szMessage )
{
    SString strMessage = GetLocalTimeString ( false, true ) + " - " + szMessage;
    if ( strMessage.length () > 0 && strMessage[ strMessage.length () - 1 ] != '\n' )
        strMessage += "\n";
#ifdef _WIN32
    OutputDebugString ( strMessage );
#else
    // Other platforms here
#endif
}
#endif


//
// Return true if supplied string adheres to the new version format
//
bool SharedUtil::IsValidVersionString ( const SString& strVersion )
{
    SString strCheck = "0.0.0-0-00000.0";
    if ( strCheck.length () != strVersion.length () )
        return false;
    for ( unsigned int i = 0 ; i < strVersion.length () ; i++ )
    {
        char c = strVersion[i];
        char d = strCheck[i];
        if ( c != d && isdigit( c ) != isdigit( d ) )
            return false;
    }
    return true;
}


//
// Try to make a path relative to the 'resources/' directory
//
SString SharedUtil::ConformResourcePath ( const char* szRes )
{
    // Remove up to first '/resources/'
    // else
    // remove up to first '/resource-cache/unzipped/'
    // else
    // remove up to first '/deathmatch/'
    // else
    // if starts with '...'
    //  remove up to first '/'

    SString strDelimList[] = { "/resources/", "/resource-cache/unzipped/", "/deathmatch/" };
    SString strText = szRes ? szRes : "";
#ifdef WIN32
    char cPathSep = '\\';
    for ( unsigned int i = 0 ; i < NUMELMS ( strDelimList ) ; i++ )
        strDelimList[i] = strDelimList[i].Replace ( "/", "\\" );
    strText = strText.Replace ( "/", "\\" );
#else
    char cPathSep = '/';
    for ( unsigned int i = 0 ; i < NUMELMS ( strDelimList ) ; i++ )
        strDelimList[i] = strDelimList[i].Replace ( "\\", "/" );
    strText = strText.Replace ( "\\", "/" );
#endif

    for ( unsigned int i = 0 ; i < NUMELMS ( strDelimList ) ; i++ )
    {
        // Remove up to first occurrence
        int iPos = strText.find ( strDelimList[i] );
        if ( iPos >= 0 )
            return strText.substr ( iPos + strDelimList[i].length () );
    }

    if ( strText.substr ( 0, 3 ) == "..." )
    {
        // Remove up to first '/'
        int iPos = strText.find ( cPathSep );
        if ( iPos >= 0 )
            return strText.substr ( iPos + 1 );
    }

    return strText;
}


namespace SharedUtil
{
    CArgMap::CArgMap ( const SString& strArgSep, const SString& strPartsSep, const SString& strExtraDisallowedChars )
        : m_strArgSep ( strArgSep )
        , m_strPartsSep ( strPartsSep )
    {
        m_strDisallowedChars = strExtraDisallowedChars + m_strArgSep + m_strPartsSep;
    }

    void CArgMap::Merge ( const CArgMap& other, bool bAllowMultiValues )
    {
        MergeFromString ( other.ToString (), bAllowMultiValues );
    }

    void CArgMap::SetFromString ( const SString& strLine, bool bAllowMultiValues )
    {
        m_Map.clear ();
        MergeFromString ( strLine, bAllowMultiValues );
    }

    void CArgMap::MergeFromString ( const SString& strLine, bool bAllowMultiValues )
    {
        std::vector < SString > parts;
        strLine.Split( m_strPartsSep, parts );
        for ( uint i = 0 ; i < parts.size () ; i++ )
        {
            SString strCmd, strArg;
            parts[i].Split ( m_strArgSep, &strCmd, &strArg );
            if ( !bAllowMultiValues )
                m_Map.erase ( strCmd );
            if ( strCmd.length () ) // Key can not be empty
                MapInsert ( m_Map, strCmd, strArg );
        }
    }

    SString CArgMap::ToString ( void ) const
    {
        SString strResult;
        for ( std::multimap < SString, SString >::const_iterator iter = m_Map.begin () ; iter != m_Map.end () ; ++iter )
        {
            if ( strResult.length () )
                strResult += m_strPartsSep;
            strResult += iter->first + m_strArgSep + iter->second;
        }
        return strResult;
    }

    bool CArgMap::HasMultiValues ( void ) const
    {
        for ( std::multimap < SString, SString >::const_iterator iter = m_Map.begin () ; iter != m_Map.end () ; ++iter )
        {
            std::vector < SString > newItems;
            MultiFind ( m_Map, iter->first, &newItems );
            if ( newItems.size () > 1 )
                return true;
        }
        return false;
    }

    void CArgMap::RemoveMultiValues ( void )
    {
        if ( HasMultiValues () )
            SetFromString ( ToString (), false );
    }

    SString CArgMap::Escape ( const SString& strIn ) const
    {
        return EscapeString ( strIn, m_strDisallowedChars );
    } 

    SString CArgMap::Unescape ( const SString& strIn ) const
    {
        return UnescapeString ( strIn );
    } 


    // Set a unique key string value
    void CArgMap::Set ( const SString& strCmd, const SString& strValue )
    {
        m_Map.erase ( Escape ( strCmd ) );
        Insert ( strCmd, strValue );
    }

    // Set a unique key int value
    void CArgMap::Set ( const SString& strCmd, int iValue )
    {
        m_Map.erase ( Escape ( strCmd ) );
        Insert ( strCmd, iValue );
    }

    // Insert a key int value
    void CArgMap::Insert ( const SString& strCmd, int iValue )
    {
        Insert ( strCmd, SString ( "%d", iValue ) );
    }

    // Insert a key string value
    void CArgMap::Insert ( const SString& strCmd, const SString& strValue )
    {
        if ( strCmd.length () ) // Key can not be empty
            MapInsert ( m_Map, Escape ( strCmd ), Escape ( strValue ) );
    }


    // Test if key exists
    bool CArgMap::Contains ( const SString& strCmd  ) const
    {
        return MapFind ( m_Map, Escape ( strCmd ) ) != NULL;
    }

    // First result as string
    bool CArgMap::Get ( const SString& strCmd, SString& strOut, const char* szDefault ) const
    {
        assert ( szDefault );
        if ( const SString* pResult = MapFind ( m_Map, Escape ( strCmd ) ) )
        {
            strOut = Unescape ( *pResult );
            return true;
        }
        strOut = szDefault;
        return false;
    }

    // First result as string
    SString CArgMap::Get ( const SString& strCmd ) const
    {
        SString strResult;
        Get ( strCmd, strResult );
        return strResult;
    }

    // All results as strings
    bool CArgMap::Get ( const SString& strCmd, std::vector < SString >& outList ) const
    {
        std::vector < SString > newItems;
        MultiFind ( m_Map, Escape ( strCmd ), &newItems );
        for ( uint i = 0 ; i < newItems.size () ; i++ )
            newItems[i] = Unescape ( newItems[i] );
        ListAppend ( outList, newItems );
        return newItems.size () > 0;
    }

    // First result as int
    bool CArgMap::Get ( const SString& strCmd, int& iValue, int iDefault ) const
    {
        SString strResult;
        if ( Get ( strCmd, strResult ) )
        {
            iValue = atoi ( strResult );
            return true;
        }
        iValue = iDefault;
        return false;
    }
}