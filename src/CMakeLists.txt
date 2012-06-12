
add_subdirectory(core)

if( NOT ${eiMODULE_LIBS} )
  #message( AllLibraries " " ${AllLibraries} )
  foreach( lib IN LISTS AllLibraries )
    AppendSources( AllSources ${lib} )
    #message( AllSources " " ${AllSources} )
  endforeach( lib IN LISTS AllLibraries )

  add_library( engine STATIC ${AllSources} )
  target_link_libraries( engine ${AllDepend} )
endif( NOT ${eiMODULE_LIBS} )

set( AllLibraries ${AllLibraries} PARENT_SCOPE )