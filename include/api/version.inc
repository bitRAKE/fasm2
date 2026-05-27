
; VERSION API calls

import version,\
       GetFileVersionInfoA,'GetFileVersionInfoA',\
       GetFileVersionInfoW,'GetFileVersionInfoW',\
       GetFileVersionInfoSizeA,'GetFileVersionInfoSizeA',\
       GetFileVersionInfoSizeW,'GetFileVersionInfoSizeW',\
       VerQueryValueA,'VerQueryValueA',\
       VerQueryValueW,'VerQueryValueW'

api GetFileVersionInfo,\
    GetFileVersionInfoSize,\
    VerQueryValue

