/*
 * Copyright (C) 2014-2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#ifndef _GAZEBO_MODEL_EDITOR_HH_
#define _GAZEBO_MODEL_EDITOR_HH_

#include <string>

#include <sdf/sdf.hh>

#include "gazebo/gui/qt.h"
#include "gazebo/gui/Editor.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class SchematicViewWidget;
    class TopToolbar;
    class ModelEditorPrivate;

    /// \class ModelEditor ModelEditor.hh gui/gui.hh
    /// \brief Interface to the terrain editor.
    class GZ_GUI_VISIBLE ModelEditor : public Editor
    {
      Q_OBJECT

      /// \brief Constuctor.
      /// \param[in] _mainWindow Pointer to the mainwindow.
      public: ModelEditor(MainWindow *_mainWindow);

      /// \brief Destuctor.
      public: virtual ~ModelEditor();

      /// \brief Add an item to palette.
      /// \param[in] _Item item to add.
      /// \param[in] _category Category to add the item too.
      public: void AddItemToPalette(QWidget *_item,
          const std::string &_category = "");

      /// \brief Spawn an entity in the editor
      /// \param[in] _sdf SDF describing the entity.
      public: void SpawnEntity(sdf::ElementPtr _sdf);

      /// \brief Remove an entity from the editor
      /// \param[in] _name Name of the entity.
      public: void RemoveEntity(const std::string &_name);

      /// \brief Get an entity in the model
      /// \param[in] _name Name of entity.
      /// \return _sdf SDF describing the entity.
      public: sdf::ElementPtr GetEntitySDF(const std::string &_name);

      public: void AppendPluginElement(const std::string &_name,
          const std::string &_filename, sdf::ElementPtr _element);

      /// \brief Qt callback when the model editor's save action is
      /// triggered.
      private slots: void Save();

      /// \brief Qt callback when the model editor's save as action is
      /// triggered.
      private slots: void SaveAs();

      /// \brief Qt callback when the model editor's new action is
      /// triggered.
      private slots: void New();

      /// \brief Qt callback when the model editor's exit action is
      /// triggered.
      private slots: void Exit();

      /// \brief QT callback when entering model edit mode
      /// \param[in] _checked True if the menu item is checked
      private slots: void OnEdit(bool _checked);

      /// \brief QT callback when the joint button is clicked.
      private slots: void OnAddSelectedJoint();

      /// \brief QT callback when a joint menu is selected
      /// \param[in] _type Type of joint.
      private slots: void OnAddJoint(const QString &_type);

      /// \brief Qt callback when a joint is added.
      private slots: void OnJointAdded();

      /// \brief Callback when an action in the toolbar has been triggered.
      /// \param[in] _action Triggered action.
      private slots: void OnAction(QAction *_action);

      /// \brief Show the schematic view widget
      /// \param[in] _show True to show the widget, false to hide it.
      private slots: void OnSchematicView(bool _show);

      /// \brief Callback when the model has been completed.
      private: void OnFinish();

      /// \brief Toggle main window's toolbar to display model editor icons.
      private: void ToggleToolbar();

      /// \brief Create menus
      private: void CreateMenus();

      /// \internal
      /// \brief Pointer to private data.
      private: ModelEditorPrivate *dataPtr;
    };
  }
}
#endif
