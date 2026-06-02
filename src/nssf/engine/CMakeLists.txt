# nssf_engine — NSSF inbound business-logic engines.
#
# SelectionEngine (selection_engine.c) implements the allowed-NSSAI decision
#   allowed = requested ∩ subscribed ∩ plmn_policy ∩ availability[tai]
# from design/nssf/module-decomposition/SelectionEngine.md. AvailabilityEngine
# (availability_engine.c) implements the NSSAIAvailability synchronous CRUD
# (replace/patch/delete/options) from
# design/nssf/module-decomposition/AvailabilityEngine.md. Both read/write through
# the AvailabilityRepository interface (nssf_repository) so they are unit-testable
# with the in-memory backend — no DB, no network. Collaborators (UDM/NRF for
# selection; the availability change-event consumer for AvailabilityEngine) are
# injected seams, so this module performs no outbound calls (M4-independent).
#
# Links nssf_wrappers for NFType anyOf handling of any nf-type query param
# (open-gaps-and-assumptions G-09) rather than re-implementing it.

add_library(nssf_engine STATIC
    selection_engine.c
    availability_engine.c)

target_include_directories(nssf_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_engine PRIVATE c_std_11)

target_link_libraries(nssf_engine
    PUBLIC
        nssf_repository
        nssf_generated
    PRIVATE
        nssf_wrappers
        nssf_sanitizers
        nssf_warnings)
