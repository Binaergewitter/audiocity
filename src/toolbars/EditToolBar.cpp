/**********************************************************************

  Audacity: A Digital Audio Editor

  EditToolBar.cpp

  Dominic Mazzoni
  Shane T. Mueller
  Leland Lucius

  See EditToolBar.h for details

*******************************************************************//*!

\class EditToolBar
\brief A ToolBar that has the edit buttons on it.

  This class, which is a child of Toolbar, creates the
  window containing interfaces to commonly-used edit
  functions that are otherwise only available through
  menus. The window can be embedded within a normal project
  window, or within a ToolBarFrame.

  All of the controls in this window were custom-written for
  Audacity - they are not native controls on any platform -
  however, it is intended that the images could be easily
  replaced to allow "skinning" or just customization to
  match the look and feel of each platform.

*//*******************************************************************/



#include "EditToolBar.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros

#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/tooltip.h>
#endif

#include "AllThemeResources.h"
#include "ImageManipulation.h"
#include "../Menus.h"
#include "Prefs.h"
#include "Project.h"
#include "UndoManager.h"
#include "../widgets/AButton.h"

#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"
#include "../commands/CommandDispatch.h"

constexpr int first_ETB_ID = 11300;

IMPLEMENT_CLASS(EditToolBar, ToolBar);

////////////////////////////////////////////////////////////
/// Methods for EditToolBar
////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( EditToolBar, ToolBar )
   EVT_COMMAND_RANGE(ETBZoomInID+first_ETB_ID,
                      ETBZoomInID+first_ETB_ID + ETBNumButtons - 1,
                      wxEVT_COMMAND_BUTTON_CLICKED,
                      EditToolBar::OnButton )
END_EVENT_TABLE()

//Standard constructor
EditToolBar::EditToolBar( AudacityProject &project )
: ToolBar(project, EditBarID, XO("Edit"), wxT("Edit"))
{
}

EditToolBar::~EditToolBar()
{
}

void EditToolBar::Create(wxWindow * parent)
{
   ToolBar::Create(parent);
   UpdatePrefs();
}

void EditToolBar::AddSeparator()
{
   mToolSizer->AddSpacer(0);
}

void EditToolBar::AddButton(
   teBmps eEnabledUp, teBmps eEnabledDown, teBmps eDisabled,
   int firstToolBarId,
   int thisButtonId,
   const TranslatableString &label,
   bool toggle)
{
   AButton *&r = mButtons[thisButtonId];

   r = ToolBarButtons::AddButton(this,
      eEnabledUp, eEnabledDown, eDisabled,
      firstToolBarId, thisButtonId,
      label, toggle);

   mToolSizer->Add(r);
}

void EditToolBar::Populate()
{
   SetBackgroundColour( theTheme.Colour( clrMedium  ) );
   MakeButtonBackgroundsSmall();

   Add(mToolSizer = safenew wxGridSizer(2, 5, 1, 1));

   /* Buttons */
   // Tooltips match menu entries.
   // We previously had longer tooltips which were not more clear.
   AddButton(bmpZoomIn, bmpZoomIn, bmpZoomInDisabled, first_ETB_ID, ETBZoomInID,
      XO("Zoom In"));
   AddButton(bmpZoomOut, bmpZoomOut, bmpZoomOutDisabled, first_ETB_ID, ETBZoomOutID,
      XO("Zoom Out"));
   AddButton(bmpZoomSel, bmpZoomSel, bmpZoomSelDisabled, first_ETB_ID, ETBZoomSelID,
      XO("Zoom to Selection"));
   AddButton(bmpZoomFit, bmpZoomFit, bmpZoomFitDisabled, first_ETB_ID, ETBZoomFitID,
      XO("Fit to Width"));

#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
   AddButton(bmpZoomToggle, bmpZoomToggle, bmpZoomToggleDisabled, first_ETB_ID, ETBZoomToggleID,
      XO("Zoom Toggle"));
#endif

   // Tooltips slightly more verbose than the menu entries are.
   AddButton(bmpTrim, bmpTrim, bmpTrimDisabled, first_ETB_ID, ETBTrimID,
      XO("Trim audio outside selection"));
   AddButton(bmpSilence, bmpSilence, bmpSilenceDisabled, first_ETB_ID, ETBSilenceID,
      XO("Silence audio selection"));

#ifdef OPTION_SYNC_LOCK_BUTTON
   AddButton(bmpSyncLockTracksUp, bmpSyncLockTracksDown, bmpSyncLockTracksUp, first_ETB_ID, ETBSyncLockID,
      XO("Sync-Lock Tracks"), true);
#else
   AddSeparator();
#endif

   AddButton(bmpUndo, bmpUndo, bmpUndoDisabled, first_ETB_ID, ETBUndoID,
      XO("Undo"));
   AddButton(bmpRedo, bmpRedo, bmpRedoDisabled, first_ETB_ID, ETBRedoID,
      XO("Redo"));

   mButtons[ETBZoomInID]->SetEnabled(false);
   mButtons[ETBZoomOutID]->SetEnabled(false);
#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
   mButtons[ETBZoomToggleID]->SetEnabled(false);
#endif

   mButtons[ETBZoomSelID]->SetEnabled(false);
   mButtons[ETBZoomFitID]->SetEnabled(false);

#ifdef OPTION_SYNC_LOCK_BUTTON
   mButtons[ETBSyncLockID]->PushDown();
#endif

   RegenerateTooltips();
}

void EditToolBar::UpdatePrefs()
{
   RegenerateTooltips();

   // Set label to pull in language change
   SetLabel(XO("Edit"));

   // Give base class a chance
   ToolBar::UpdatePrefs();
}

void EditToolBar::RegenerateTooltips()
{
   ForAllButtons( TBActTooltips );
}

void EditToolBar::EnableDisableButtons()
{
   ForAllButtons( TBActEnableDisable );
}

static const struct Entry {
   int tool;
   CommandID commandName;
   TranslatableString untranslatedLabel;
} EditToolbarButtonList[] = {
   { ETBZoomInID,   wxT("ZoomIn"),      XO("Zoom In")  },
   { ETBZoomOutID,  wxT("ZoomOut"),     XO("Zoom Out")  },
#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
   { ETBZoomToggleID,   wxT("ZoomToggle"),      XO("Zoom Toggle")  },
#endif
   { ETBZoomSelID,  wxT("ZoomSel"),     XO("Fit selection to width")  },
   { ETBZoomFitID,  wxT("FitInWindow"), XO("Fit project to width")  },

   { ETBTrimID,     wxT("Trim"),        XO("Trim audio outside selection")  },
   { ETBSilenceID,  wxT("Silence"),     XO("Silence audio selection")  },
#ifdef OPTION_SYNC_LOCK_BUTTON
   { ETBSyncLockID, wxT("SyncLock"),    XO("Sync-Lock Tracks")  },
#endif
   { ETBUndoID,     wxT("Undo"),        XO("Undo")  },
   { ETBRedoID,     wxT("Redo"),        XO("Redo")  },
};

void EditToolBar::ForAllButtons(int Action)
{
   AudacityProject *p;
   CommandManager* cm = nullptr;

   if( Action & TBActEnableDisable ){
      p = &mProject;
      cm = &CommandManager::Get( *p );
#ifdef OPTION_SYNC_LOCK_BUTTON
      bool bSyncLockTracks;
      gPrefs->Read(wxT("/GUI/SyncLockTracks"), &bSyncLockTracks, false);

      if (bSyncLockTracks)
         mButtons[ETBSyncLockID]->PushDown();
      else
         mButtons[ETBSyncLockID]->PopUp();
#endif
   }


   for (const auto &entry : EditToolbarButtonList) {
#if wxUSE_TOOLTIPS
      if( Action & TBActTooltips ){
         ComponentInterfaceSymbol command{
            entry.commandName, entry.untranslatedLabel };
         ToolBar::SetButtonToolTip( mProject,
            *mButtons[entry.tool], &command, 1u );
      }
#endif
      if (cm) {
         mButtons[entry.tool]->SetEnabled(cm->GetEnabled(entry.commandName));
      }
   }
}

void EditToolBar::OnButton(wxCommandEvent &event)
{
   int id = event.GetId()-first_ETB_ID;
   // Be sure the pop-up happens even if there are exceptions, except for buttons which toggle.
   auto cleanup = finally( [&] { mButtons[id]->InteractionOver();});

   AudacityProject *p = &mProject;
   auto &cm = CommandManager::Get( *p );

   auto flags = MenuManager::Get(*p).GetUpdateFlags();
   const CommandContext context( *p );
   ::HandleTextualCommand( cm,
      EditToolbarButtonList[id].commandName, context, flags, false);

#if defined(__WXMAC__)
   // Bug 2402
   // LLL: It seems that on the Mac the IDLE events are processed
   //      differently than on Windows/GTK and the AdornedRulerPanel's
   //      OnPaint() method gets called sooner that expected. This is
   //      evident when zooming from this toolbar only. When zooming from
   //      the Menu or from keyboard ommand, the zooming works correctly.
   wxTheApp->ProcessIdle();
#endif
}

static RegisteredToolbarFactory factory{ EditBarID,
   []( AudacityProject &project ){
      return ToolBar::Holder{ safenew EditToolBar{ project } }; }
};

#include "ToolManager.h"

namespace {
AttachedToolBarMenuItem sAttachment{
   /* i18n-hint: Clicking this menu item shows the toolbar for editing */
   EditBarID, wxT("ShowEditTB"), XXO("&Edit Toolbar")
};
}

