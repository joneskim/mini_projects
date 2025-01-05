file(REMOVE_RECURSE
  "libdb_lib.a"
  "libdb_lib.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/db_lib.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
