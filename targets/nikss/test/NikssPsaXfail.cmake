# Wildcard failing tests for now. TODO:  Classify further.
p4tools_add_xfail_reason(
  "flay-nikss-psa"
  "Compiler Bug|Unimplemented compiler support"
  register-structs.p4 # Unimplemented compiler support: Unsupported assignment type struct
)

p4tools_add_xfail_reason(
  "flay-nikss-psa"
  "unsupported argument expression"
  # Issue with some control-plane code.
  const-entry-and-action.p4
  const-entry-ternary.p4
  const-entry.p4
)
