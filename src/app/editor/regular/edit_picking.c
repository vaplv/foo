#include "app/core/app.h"
#include "app/core/app_core.h"
#include "app/core/app_world.h"
#include "app/editor/regular/edit_context_c.h"
#include "app/editor/regular/edit_error_c.h"
#include "app/editor/regular/edit_picking.h"
#include "app/editor/edit_model_instance_selection.h"

/*******************************************************************************
 *
 * Helper functions
 *
 ******************************************************************************/

/*******************************************************************************
 *
 * Picking functions
 *
 ******************************************************************************/
enum edit_error
edit_process_picking(struct edit_context* ctxt)
{
  const app_pick_t* pick_list = NULL;
  size_t nb_picks = 0;
  enum edit_error edit_err = EDIT_NO_ERROR;

  if(UNLIKELY(!ctxt)) {
    edit_err = EDIT_INVALID_ARGUMENT;
    goto error;
  }
  
  APP(poll_picking(ctxt->app, &nb_picks, &pick_list));
  if(nb_picks) {
    struct app_world* world = NULL;
    size_t i = 0;
    enum app_error app_err = APP_NO_ERROR;

    app_err = app_get_main_world(ctxt->app, &world);
    if(app_err != APP_NO_ERROR) {
      edit_err = app_to_edit_error(app_err);
      goto error;
    }
    #define CALL(func) if(EDIT_NO_ERROR != (edit_err = func)) goto error
    for(i = 0; i < nb_picks; ++i) {
      struct app_model_instance* instance = NULL;
      const uint32_t pick_id = APP_PICK_ID_GET(pick_list[i]);
      const enum app_pick_group pick_group = APP_PICK_GROUP_GET(pick_list[i]);
      bool is_selected = false;

      if(pick_group == APP_PICK_GROUP_IMDRAW) {
        printf("--%d\n", pick_id);
      } else if(pick_group == APP_PICK_GROUP_WORLD) {

        if(pick_id >= APP_PICK_ID_MAX) {
          if(nb_picks == 1) {
            CALL(edit_clear_model_instance_selection(ctxt->instance_selection));
          }
        } else {
          app_err = app_world_picked_model_instance
            (world, pick_list[i], &instance);
          if(app_err != APP_NO_ERROR) {
            edit_err = app_to_edit_error(app_err);
            goto error;
          }
          CALL(edit_is_model_instance_selected
            (ctxt->instance_selection, instance, &is_selected));
          if(is_selected) {
            CALL(edit_unselect_model_instance
              (ctxt->instance_selection,instance));
          } else {
            CALL(edit_select_model_instance
              (ctxt->instance_selection, instance));
          }
        }
      }
    }
    #undef CALL
  }

exit:
  return edit_err;
error:
  goto exit;
}

