cmake_minimum_required(VERSION 2.8.8)
 
cmake_policy(SET CMP0015 OLD)

######## Build Types ########

#set( CMAKE_CONFIGURATION_TYPES Debug Dev Retail )
set( CMAKE_CONFIGURATION_TYPES "Debug;Dev;Retail" CACHE STRING "" FORCE )

project (Eight)

set( CMAKE_DEBUG_POSTFIX _debug )
set( CMAKE_DEV_POSTFIX   _dev )

set( CMAKE_USE_RELATIVE_PATHS TRUE )

set( CMAKE_INSTALL_PREFIX           ${CMAKE_HOME_DIRECTORY}/bin CACHE PATH "Build directory" FORCE)
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin CACHE PATH "Build directory" FORCE) #exe
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin CACHE PATH "Build directory" FORCE) #dll
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib CACHE PATH "Lib directory" FORCE) #lib
######## Options ########

set( eiMODULE_LIBS 0 )

######## Compiler Support ########

#Configuration for Microsoft Visual Studio
if( MSVC )
  set( BaseDefines eiBUILD_MSVC )
  set( BaseDefines ${BaseDefines} eiBUILD_WINDOWS )
  
  # TODO - if building for x64 CPUs, define eiBUILD_X64
  
  # Let us use the standard library (and don't debug it)
  set( BaseDefines ${BaseDefines} _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE _SECURE_SCL=0 _HAS_TR1=0 )

  # Set a build macro for each build type
  set( DbgDefines ${BaseDefines} eiBUILD_DEBUG  eiDEBUG_LEVEL=2 _DEBUG )
  set( DevDefines ${BaseDefines} eiBUILD_DEV    eiDEBUG_LEVEL=1 NDEBUG )
  set( RelDefines ${BaseDefines} eiBUILD_RETAIL eiDEBUG_LEVEL=0 NDEBUG )
  set_directory_properties( PROPERTIES COMPILE_DEFINITIONS_DEBUG  "${DbgDefines}" )
  set_directory_properties( PROPERTIES COMPILE_DEFINITIONS_DEV    "${DevDefines}" )
  set_directory_properties( PROPERTIES COMPILE_DEFINITIONS_RETAIL "${RelDefines}" )


  # No exception handling
  set( BaseFlags /EHsc- )

  # No RTTI
  set( BaseFlags /GR- )
  
  # No buffer security checks
  set( BaseFlags /GS- )

  # SIMD instruction minimum requirement
  set( BaseFlags "${BaseFlags} /arch:SSE2" )

  # Fast (non standards compliant) float math please
  set( BaseFlags "${BaseFlags} /fp:fast" )
  
  # Strict Warnings
  set( BaseFlags "${BaseFlags} /W3" )
  set( BaseFlags "${BaseFlags} /WX" )
  
  # Enable intrinsics
  set( BaseFlags "${BaseFlags} /Oi" )
  
  # Ignore warning C4355: 'this' : used in base member initializer list
  set( BaseFlags "${BaseFlags} /wd4355" )

  #no exception handling
  set( BaseFlags "${BaseFlags} /EHsc-" )

  #no RTTI
  set( BaseFlags "${BaseFlags} /GR-" )

  
  # Copy all the above to release and debug builds
  set( RelFlags "${BaseFlags}" )
  set( DbgFlags "${BaseFlags}" )

  # inlining and optimisation
  set( RelFlags "${RelFlags} /Ob2 /O2" )
  
  # debugging database (/Zi)
  set( RelFlags "${RelFlags} /Zi" )
  
  # Clone release into Dev
  set( DevFlags "${RelFlags}" )
  
  # Minimal rebuild, no LTCG
  set( DevFlags "${DevFlags} /Gm /GL-" )
  
  # LTCG, Omit frame pointers, string pooling
  set( RelFlags "${RelFlags} /GL /Oy /GF" )
  
  # Debug has database with edit and continue and function level linking
  set( DbgFlags "${DbgFlags} /ZI /Gy" )
  
  # Runtime checks, buffer security checks, conversion to smaller type check, 
  set( DbgFlags "${DbgFlags} /RTC1 /GS /RTCc" )

  # No optimisation
  set( DbgFlags "${DbgFlags} /Od" )
  
  # MSVC runtime library
  set( RelFlags "${RelFlags} /MT"  ) # static lib
  set( DevFlags "${DevFlags} /MT"  ) # static lib UNDONE: /MD shared dll
  set( DbgFlags "${DbgFlags} /MDd" ) # debug dll
  
  #add_definitions( "${DevFlags}" )
  #damn, this doesn't work for flags (/bla)
  #set_directory_properties( PROPERTIES COMPILE_DEFINITIONS_DEBUG  "${DevFlags}" )
  #set_directory_properties( PROPERTIES COMPILE_DEFINITIONS_DEV    "${DevFlags}" )
  #set_directory_properties( PROPERTIES COMPILE_DEFINITIONS_RETAIL "${RelFlags}" )

  set( CMAKE_CXX_FLAGS_DEV    ${DevFlags} )
  set( CMAKE_CXX_FLAGS_DEBUG  ${DbgFlags} )
  set( CMAKE_CXX_FLAGS_RETAIL ${RelFlags} )
  
  set( DbgLinkFlags "/INCREMENTAL" )
  set( DevLinkFlags "/MANIFEST:NO /INCREMENTAL" )
  set( RelLinkFlags "/MANIFEST:NO /INCREMENTAL:NO /LTCG /OPT:REF" )
  set( DbgLibFlags "/INCREMENTAL" )
  set( DevLibFlags "/INCREMENTAL" )
  set( RelLibFlags "/LTCG" )

  set( CMAKE_EXE_LINKER_FLAGS_RETAIL    "${RelLinkFlags}" )
  set( CMAKE_EXE_LINKER_FLAGS_DEV       "${DevLinkFlags}" )
  set( CMAKE_EXE_LINKER_FLAGS_DEBUG     "${DbgLinkFlags}" )

  # Mark inline headers as a different category/type in the file tree
  #source_group( "Inline Files" REGULAR_EXPRESSION .*\\.hpp )
  #source_group( "CMake" REGULAR_EXPRESSION .*CMake.* )
  #source_group( "ISPC Headers" REGULAR_EXPRESSION .*ispc\\.h )
endif( MSVC )



######## Project Macros ########

macro( AddSubDir name )
  list( APPEND SubDirs ${CMAKE_CURRENT_SOURCE_DIR}/${name} )
  list( APPEND SubDirs ${include}/${eiCURRENT_LIB_PATH}/${name} )
endmacro( AddSubDir name )

macro( AddSourceGroup name )
  set( temp )
  foreach( item ${ARGN} )
    set( var pathTo{$name}${eiCURRENT_LIB_PATH}${item} )
    find_file(${var} NAMES ${item} PATHS ${CMAKE_CURRENT_SOURCE_DIR} ${include}/${eiCURRENT_LIB_PATH} ${SubDirs} NO_DEFAULT_PATH )
    #message( "${var} = ${${var}}  - ${item} in ${CMAKE_CURRENT_SOURCE_DIR}, ${include}/${eiCURRENT_LIB_PATH}, ${SubDirs}" )
	string(FIND "${${var}}" ".ispc" isIspc)
	if(${isIspc} STREQUAL "-1" OR NOT ${eiMODULE_LIBS})
		list( APPEND temp ${${var}} )
	else()
		AppendIspcSrc( temp ${item} ${${var}} )
	endif()
  endforeach( item ${ARGN} )
  set( SoruceGroup${eiCURRENT_LIB_PATH}${name} ${temp} )
  set( SoruceGroup${eiCURRENT_LIB_PATH}${name} ${temp} PARENT_SCOPE )
  list( APPEND LibrarySoruceGroups${eiCURRENT_LIB_PATH} ${name} )
endmacro( AddSourceGroup name )

macro( BeginLibrary path )
  set( eiCURRENT_LIB_PATH ${path} )
  if( ${eiMODULE_LIBS} )
    SourceGroup( ${path} ${path} )
    set( Depend ${ARGN} )
  else( ${eiMODULE_LIBS} )
    set( eiCURRENT_LIB_SLASH ${path}\\ )
    foreach( item ${ARGN} )
      list( FIND AllLibraries ${item} index )
      if( ${index} EQUAL -1 )
        list( APPEND Depend ${item} )
      endif( ${index} EQUAL -1 )
    endforeach( item ${ARGN} )
  endif( ${eiMODULE_LIBS} )
endmacro( BeginLibrary )

macro( EndLibrary )
  if( NOT DEFINED SoruceGroup${eiCURRENT_LIB_PATH}${eiCURRENT_LIB_PATH} )
    AddSourceGroup( ${eiCURRENT_LIB_PATH} )
  endif( NOT DEFINED SoruceGroup${eiCURRENT_LIB_PATH}${eiCURRENT_LIB_PATH} )
  set( LibrarySoruceGroups${eiCURRENT_LIB_PATH} ${LibrarySoruceGroups${eiCURRENT_LIB_PATH}} PARENT_SCOPE )
    
  if( ${eiMODULE_LIBS} )
    #AddLibrary( ${eiCURRENT_LIB_PATH} ${Sources} ${Headers} )
    
    set( temp )
    AppendSources( temp ${eiCURRENT_LIB_PATH} )
    source_group( "" FILES CMakeLists.txt )
    add_library( ${eiCURRENT_LIB_PATH} STATIC ${temp} )
    set_target_properties( ${eiCURRENT_LIB_PATH} PROPERTIES STATIC_LIBRARY_FLAGS_RETAIL ${RelLibFlags} )
    set_target_properties( ${eiCURRENT_LIB_PATH} PROPERTIES STATIC_LIBRARY_FLAGS_DEV    ${DevLibFlags} )
    set_target_properties( ${eiCURRENT_LIB_PATH} PROPERTIES STATIC_LIBRARY_FLAGS_DEBUG  ${DbgLibFlags} )
    if( DEFINED Depend )
      target_link_libraries( ${eiCURRENT_LIB_PATH} ${Depend} )
    endif( DEFINED Depend )
  else( ${eiMODULE_LIBS} )
    #list( APPEND AllSources ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt )
    list( APPEND AllLibraries ${eiCURRENT_LIB_PATH} )
    list( APPEND AllDepend ${Depend} )
    set( AllSources ${AllSources} PARENT_SCOPE )
    set( AllLibraries ${AllLibraries} PARENT_SCOPE )
    set( AllDepend ${AllDepend} PARENT_SCOPE )
  endif( ${eiMODULE_LIBS} )
endmacro( EndLibrary )

macro( AppendSources out lib )
    foreach( group IN LISTS LibrarySoruceGroups${lib} )
      set( groupFiles ${SoruceGroup${lib}${group}} )
      ProcessGroup( groupFiles )
      if( ${lib} STREQUAL ${group} )
        set( groupName "${lib}" )
        if( ${eiMODULE_LIBS} )
          list( APPEND groupFiles CMakeLists.txt )
        else( ${eiMODULE_LIBS} )
          list( APPEND groupFiles ${lib}/CMakeLists.txt )
        endif( ${eiMODULE_LIBS} )
      else( ${lib} STREQUAL ${group} )
        set( groupName "${lib}\\${group}" )
      endif( ${lib} STREQUAL ${group} )
      source_group( ${groupName} FILES ${groupFiles} )
      list( APPEND ${out} ${groupFiles} )
    endforeach( group IN LISTS AllLibraries )
endmacro( AppendSources out lib )

macro( ProcessGroup files )
  set( newFiles )
  foreach( file IN LISTS ${files} )
    string(FIND "${file}" ".ispc" isIspc)
    if(NOT ${isIspc} STREQUAL "-1" )
      #set_source_files_properties( ${file} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE )
      string(REPLACE "${CMAKE_CURRENT_LIST_DIR}/" "" fileShort ${file} )
		message( "f1 " ${file} )
		message( "f2 " ${fileShort} )
		SetupIspcSrc( ${fileShort} ${file} hdrLocation )
		list( APPEND newFiles ${hdrLocation} )
    endif()
  endforeach( file IN LISTS ${files} )
  list( APPEND ${files} ${newFiles} )
endmacro( ProcessGroup )

macro( AddIspcSrc outList srcLocation )
	set(srcLocation2 ${srcLocation})
	find_file(srcLocation2 NAMES ${srcLocation} PATHS ${CMAKE_CURRENT_SOURCE_DIR} )
	get_source_file_property(srcLocation2 ${srcLocation2} LOCATION)
	AppendIspcSrc( ${outList} ${srcLocation} ${srcLocation2} )
endmacro( AddIspcSrc )

macro( AppendIspcSrc outList srcLocation srcLocation2 )
	SetupIspcSrc( ${srcLocation} ${srcLocation2} hdrLocation2 )
	list( APPEND ${outList} ${srcLocation2} ${hdrLocation2} )
endmacro( AppendIspcSrc )

macro( SetupIspcSrc srcLocation srcLocation2 outHdrVar )
	string(FIND "${srcLocation}" "/" lastSlash REVERSE) #just get the file name, strip the path
	if(NOT ${lastSlash} STREQUAL "-1" )
		math(EXPR lastSlash ${lastSlash}+1)
		string(SUBSTRING "${srcLocation}" ${lastSlash} -1 objLocation)
		#string(SUBSTRING "${srcLocation}" 0 ${lastSlash} currentDir)
	else()
		set( objLocation ${srcLocation} )
		#set( currentDir ${CMAKE_CURRENT_LIST_DIR} )
	endif()
	set( currentDir ${CMAKE_CURRENT_LIST_DIR} )
	string(REPLACE ".ispc" ".obj" objLocation ${objLocation})
	set(objLocation ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/ispc/${objLocation}) # put the obj in the intermediate dir

	string(REPLACE ".ispc" "_ispc.h" hdrLocation ${srcLocation})

	set( hdrLocation2 ${currentDir}/${hdrLocation} )
	#get_source_file_property(hdrLocation2 ${hdrLocation} LOCATION)
	message( "h " ${hdrLocation2} )
	message( "o " ${objLocation} )
	message( "c " ${currentDir} )

	ADD_CUSTOM_COMMAND(
	   OUTPUT ${objLocation} ${hdrLocation2}
	   COMMAND ${ISPC_EXE} $(ConfigurationName) ${srcLocation2} -o ${objLocation} -h ${hdrLocation} -I ${CMAKE_SOURCE_DIR}/include
	   IMPLICIT_DEPENDS C ${srcLocation}
	   DEPENDS ${srcLocation}
	   MAIN_DEPENDENCY  ${srcLocation}  # this gives the ispc file in MSVC a 'compile' option on the source file
           WORKING_DIRECTORY ${currentDir}
	   )
	
	set_source_files_properties( ${objLocation} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE )
	#set_source_files_properties( ${hdrLocation} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE )
	set_source_files_properties( ${hdrLocation2} PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE )

	#source_group( "ISPC Headers" FILES ${hdrLocation2} )
	
	set( ${outHdrVar} ${hdrLocation2} )
endmacro( SetupIspcSrc )


macro( SetTargetProperties target_name )
  set_target_properties( ${target_name} PROPERTIES DEBUG_PREFIX dbg )
  set_target_properties( ${target_name} PROPERTIES   DEV_PREFIX dev )
endmacro( SetTargetProperties )

macro( SourceGroup name path )
  source_group( "${name}"  REGULAR_EXPRESSION .*/${path}/.* )
endmacro( SourceGroup )

macro( SourceSubGroup name path )
  if( ${eiMODULE_LIBS} OR NOT DEFINED eiCURRENT_LIB_SLASH )
    source_group( "${eiCURRENT_LIB_SLASH}${name}"  REGULAR_EXPRESSION .*/${path}.* )
  else( ${eiMODULE_LIBS} OR NOT DEFINED eiCURRENT_LIB_SLASH )
    list( APPEND SourceGroups "${eiCURRENT_LIB_SLASH}${name}" ".*${eiCURRENT_LIB_PATH}/${path}.*" ) 
    set( SourceGroups ${SourceGroups} PARENT_SCOPE )
  endif( ${eiMODULE_LIBS} OR NOT DEFINED eiCURRENT_LIB_SLASH )
endmacro( SourceSubGroup )

macro( Depends lib base )
  if( ${eiMODULE_LIBS} )
    target_link_libraries( ${lib} ${base} )
  else( ${eiMODULE_LIBS} )
  endif( ${eiMODULE_LIBS} )
endmacro( Depends )

macro( AddLibrary lib_name Sources Headers )
  source_group( "" FILES CMakeLists.txt )
  add_library( ${lib_name} STATIC ${${Sources}} ${${Headers}} )
  set_target_properties( ${lib_name} PROPERTIES STATIC_LIBRARY_FLAGS_RETAIL ${RelLibFlags} )
  set_target_properties( ${lib_name} PROPERTIES STATIC_LIBRARY_FLAGS_DEV    ${DevLibFlags} )
  set_target_properties( ${lib_name} PROPERTIES STATIC_LIBRARY_FLAGS_DEBUG  ${DbgLibFlags} )
endmacro( AddLibrary )

macro( AddTest test_name test_app depend_libs )
  source_group( "" FILES ${test_app} )
  add_executable       ( ${test_name} ${test_app} )
  SetTargetProperties  ( ${test_name} )
  add_custom_command( TARGET   ${test_name} POST_BUILD
                      COMMAND  ${test_name} 
                      COMMENT  "Launching test: ${test_name}" )
                      
  if( ${eiMODULE_LIBS} )
    add_dependencies     ( ${test_name} ${depend_libs} )
    target_link_libraries( ${test_name} ${depend_libs} )
  else( ${eiMODULE_LIBS} )
    foreach( item ${depend_libs} )
      list( FIND AllLibraries ${item} index )
      if( ${index} EQUAL -1 )
        add_dependencies     ( ${test_name} ${item} )
        target_link_libraries( ${test_name} ${item} )
      else( ${index} EQUAL -1 )
        add_dependencies     ( ${test_name} engine )
        target_link_libraries( ${test_name} engine )
      endif( ${index} EQUAL -1 )
    endforeach( item ${ARGN} )
  endif( ${eiMODULE_LIBS} )
endmacro( AddTest )

macro( InstallAwesomium target )
  add_custom_command( TARGET  ${target} POST_BUILD
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/debug/icudt42.dll\"              \"${CMAKE_SOURCE_DIR}/bin/debug/\"          /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/icudt42.dll\"            \"${CMAKE_SOURCE_DIR}/bin/dev/\"            /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/icudt42.dll\"            \"${CMAKE_SOURCE_DIR}/bin/retail/\"         /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/debug/Awesomium_d.dll\"          \"${CMAKE_SOURCE_DIR}/bin/debug/\"          /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/Awesomium.dll\"          \"${CMAKE_SOURCE_DIR}/bin/dev/\"            /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/Awesomium.dll\"          \"${CMAKE_SOURCE_DIR}/bin/retail/\"         /i /y /D /F
#                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/debug/AwesomiumProcess_d.exe\"   \"${CMAKE_SOURCE_DIR}/bin/debug/\"          /i /y /D /F
#                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/AwesomiumProcess.exe\"   \"${CMAKE_SOURCE_DIR}/bin/dev/\"            /i /y /D /F
#                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/AwesomiumProcess.exe\"   \"${CMAKE_SOURCE_DIR}/bin/retail/\"         /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/debug/locales/en-US.dll\"        \"${CMAKE_SOURCE_DIR}/bin/debug/locales/\"  /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/locales/en-US.dll\"      \"${CMAKE_SOURCE_DIR}/bin/dev/locales/\"    /i /y /D /F
                      COMMAND  XCOPY \"${CMAKE_SOURCE_DIR}/external/awesomium/1.6.3/build/bin/release/locales/en-US.dll\"      \"${CMAKE_SOURCE_DIR}/bin/retail/locales/\" /i /y /D /F
                      COMMENT  "Installing Awesomium for ${target}" )
endmacro( InstallAwesomium )



macro( AddApp Name Sources )
  source_group( "" FILES ${Name} )
  add_executable       ( ${Name} ${${Sources}} )
  #add_dependencies     ( ${Name} ${Depends} )
  #target_link_libraries( ${Name} ${Depends} )
  SetTargetProperties  ( ${Name} )
  
  if( ${eiMODULE_LIBS} )
    add_dependencies     ( ${Name} ${ARGN} )
    target_link_libraries( ${Name} ${ARGN} )
  else( ${eiMODULE_LIBS} )
    foreach( item ${ARGN} )
      list( FIND AllLibraries ${item} index )
      if( ${index} EQUAL -1 )
        add_dependencies     ( ${Name} ${item} )
        target_link_libraries( ${Name} ${item} )
      else( ${index} EQUAL -1 )
        add_dependencies     ( ${Name} engine )
        target_link_libraries( ${Name} engine )
      endif( ${index} EQUAL -1 )
    endforeach( item ${ARGN} )
  endif( ${eiMODULE_LIBS} )
endmacro( AddApp )


######## ISPC Support ########

set( ISPC_EXE $(SolutionDir)/../external/ispc/ispc.bat )
#set( ISPC_FLAGS --target=sse2 --arch=x86 )

######## Project Global Variables ########

set( include ${CMAKE_SOURCE_DIR}/include/eight ) # base dir for public headers


add_library( sockets            STATIC IMPORTED )
add_library( luajit             STATIC IMPORTED )

set_target_properties( sockets PROPERTIES
	IMPORTED_LOCATION_DEBUG  "Ws2_32.lib"
	IMPORTED_LOCATION_DEV    "Ws2_32.lib"
	IMPORTED_LOCATION_RETAIL "Ws2_32.lib" )
set_target_properties( luajit PROPERTIES
	IMPORTED_LOCATION_DEBUG  "${CMAKE_SOURCE_DIR}/external/LuaJIT/src/lua51.lib"
	IMPORTED_LOCATION_DEV    "${CMAKE_SOURCE_DIR}/external/LuaJIT/src/lua51.lib"
	IMPORTED_LOCATION_RETAIL "${CMAKE_SOURCE_DIR}/external/LuaJIT/src/lua51.lib" )

include_directories (external)

######## Build the source ########

include_directories (include)
   link_directories (lib    )
   add_subdirectory (src    )
