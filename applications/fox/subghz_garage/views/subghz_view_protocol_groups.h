#pragma once
#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SubGhzProtocolGroups SubGhzProtocolGroups;

/* Fired when OK confirms a new active group (Up/Down only move the cursor
 * highlight - the RX group doesn't switch until OK is pressed on it). */
typedef void (*SubGhzProtocolGroupsCallback)(void* context, uint8_t group_index);

SubGhzProtocolGroups* subghz_protocol_groups_alloc(void);
void subghz_protocol_groups_free(SubGhzProtocolGroups* instance);
View* subghz_protocol_groups_get_view(SubGhzProtocolGroups* instance);
void subghz_protocol_groups_set_callback(
    SubGhzProtocolGroups* instance,
    SubGhzProtocolGroupsCallback callback,
    void* context);
void subghz_protocol_groups_set_selected(SubGhzProtocolGroups* instance, uint8_t group_index);

/* The scroll-text timer only needs to run while this view is actually the
 * one on screen - call resume from the scene's on_enter and pause from
 * on_exit. Safe to call redundantly (e.g. pause when already paused). */
void subghz_protocol_groups_resume_scroll(SubGhzProtocolGroups* instance);
void subghz_protocol_groups_pause_scroll(SubGhzProtocolGroups* instance);

#ifdef __cplusplus
}
#endif
