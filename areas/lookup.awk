#
# Searchs input file for the "target" object
# Prints object number and line matching "target"
# Assumes the object/mob file format is :
#   <#number>
#   <short_desc>
#   <slightly longer desc>  <-- we save this one.
#
# ie. #6902
#     staff~
#     Dionysos' staff of Wonders~
#
# Invocation : gawk -f lookup.gawk target=<value> <input file> > output file
#
# nawk can be used for gawk.
#
BEGIN     { IGNORECASE = 1 }
/#[0-9]+/    { prev = $0 
	       getline
	       getline
	       if ($0 ~ target) {
	          printf "%s %s\n", prev, $0;
               }
             }
	{}
