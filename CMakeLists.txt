set(COMPONENT_SRCS
  nifs/atomvm_adc.c
  )

set(COMPONENT_ADD_INCLUDEDIRS
  nifs/include
  )

set(COMPONENT_REQUIRES nifs)

register_component()
