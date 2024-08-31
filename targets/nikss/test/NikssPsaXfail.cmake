# Wildcard failing tests for now. TODO:  Classify further.
p4tools_add_xfail_reason(
  "flay-nikss-psa"
  "Compiler Bug|Unimplemented compiler support"
  register-structs.p4 # Unimplemented compiler support: Unsupported assignment type struct
)
