/*!
	@file
	@author		Georgiy Evmenov
	@date		09/2008
	@module
*/
#include "PropertiesPanelView.h"
#include "WidgetContainer.h"
#include "WidgetTypes.h"
#include "UndoManager.h"
#include "Parse.h"

#define ON_EXIT( CODE ) class _OnExit { public: ~_OnExit() { CODE; } } _onExit

int grid_step;//FIXME_HOOK
int toGrid(int _x){ return _x / grid_step * grid_step; }

const std::string DEFAULT_VALUE = "#444444[DEFAULT]";

inline const Ogre::UTFString localise(const Ogre::UTFString & _str)
{
	return MyGUI::LanguageManager::getInstance().getTag(_str);
}

PropertiesPanelView::PropertiesPanelView()
{
}

void PropertiesPanelView::initialise()
{
	mLayoutName = "PropertiesPanelView.layout";
	
	PanelViewWindow::initialise();

	addItem(&mPanelMainProperties);
	mPanelMainProperties.eventCreatePair = MyGUI::newDelegate(this, &PropertiesPanelView::createPropertiesWidgetsPair);
	mPanelMainProperties.eventSetPositionText = MyGUI::newDelegate(this, &PropertiesPanelView::setPositionText);
	addItem(&mPanelTypeProperties);
	mPanelTypeProperties.eventCreatePair = MyGUI::newDelegate(this, &PropertiesPanelView::createPropertiesWidgetsPair);
	addItem(&mPanelGeneralProperties);
	mPanelGeneralProperties.eventCreatePair = MyGUI::newDelegate(this, &PropertiesPanelView::createPropertiesWidgetsPair);
	addItem(&mPanelEvents);
	mPanelEvents.eventCreatePair = MyGUI::newDelegate(this, &PropertiesPanelView::createPropertiesWidgetsPair);
	addItem(&mPanelItems);
	addItem(&mPanelUserData);

	mPanels.push_back(mPanelMainProperties);
	mPanels.push_back(mPanelTypeProperties);
	mPanels.push_back(mPanelGeneralProperties);
	mPanels.push_back(mPanelEvents);
	mPanels.push_back(mPanelItems);
	mPanels.push_back(mPanelUserData);

	// create widget rectangle
	current_widget_rectangle = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("StretchRectangle", MyGUI::IntCoord(), MyGUI::Align::Default, "LayoutEditor_Rectangle");
	current_widget_rectangle->eventWindowChangeCoord = newDelegate(this, &PropertiesPanelView::notifyRectangleResize);
	current_widget_rectangle->eventMouseButtonDoubleClick = newDelegate(&mPanelItems, &PanelItems::notifyRectangleDoubleClick);
	current_widget_rectangle->eventKeyButtonPressed = newDelegate(this, &PropertiesPanelView::notifyRectangleKeyPressed);

	arrow_move = false;
}


void PropertiesPanelView::load(MyGUI::xml::xmlNodeIterator _field)
{
	MyGUI::xml::xmlNodeIterator field = _field->getNodeIterator();
	std::vector<PanelBase>::iterator iter = mPanels.begin();
	while (field.nextNode()) {
		std::string key, value;

		if (field->getName() == "Property")
		{
			if (false == field->findAttribute("key", key)) continue;
			if (false == field->findAttribute("value", value)) continue;

			if ((key == MyGUI::utility::toString("Panel"/*, i*/,"Minimized")) && (iter != mPanels.end()))
			{
				(*iter).getPanelCell()->setMinimized(MyGUI::utility::parseBool(value));
				++iter;
			}
		}
	}
}

void PropertiesPanelView::save(MyGUI::xml::xmlNodePtr root)
{
	root = root->createChild("PropertiesPanelView");
	MyGUI::xml::xmlNodePtr nodeProp;

	int i = 0;
	for (std::vector<PanelBase>::iterator iter = mPanels.begin(); iter != mPanels.end(); ++iter, ++i)
	{
		nodeProp = root->createChild("Property");
		nodeProp->addAttributes("key", MyGUI::utility::toString("Panel"/*, i*/,"Minimized"));
		nodeProp->addAttributes("value", (*iter).getPanelCell()->isMinimized());
	}
}

void PropertiesPanelView::notifyRectangleResize(MyGUI::WidgetPtr _sender)
{
	if (!_sender->isShow()) return;
	// ������ ��������������� ��������� ������� � ����������/��������
	WidgetContainer * widgetContainer = EditorWidgets::getInstance().find(current_widget);
	if (WidgetTypes::getInstance().find(current_widget->getTypeName())->resizeable)
	{
		MyGUI::IntCoord coord = _sender->getCoord();
		MyGUI::IntCoord old_coord = current_widget->getAbsoluteCoord();
		// align to grid
		if (!MyGUI::InputManager::getInstance().isShiftPressed() && !arrow_move){
			if ((old_coord.width == coord.width) && (old_coord.height == coord.height)) // ���� ������ ����������
			{
				coord.left = toGrid(coord.left + grid_step-1 - old_coord.left) + old_coord.left;
				coord.top = toGrid(coord.top + grid_step-1 - old_coord.top) + old_coord.top;
			}
			else // ���� �����������
			{
				if (old_coord.left != coord.left){
					coord.left = toGrid(coord.left + grid_step-1);
					coord.width = old_coord.right() - coord.left;
				}else if (old_coord.width != coord.width){
					coord.width = toGrid(coord.width + old_coord.left) - old_coord.left;
				}

				if (old_coord.top != coord.top){
					coord.top = toGrid(coord.top + grid_step-1);
					coord.height = old_coord.bottom() - coord.top;
				}else if (old_coord.height != coord.height){
					coord.height = toGrid(coord.height + old_coord.top) - old_coord.top;
				}
			}
		}
		arrow_move = false;

		coord = convertCoordToParentCoord(coord, current_widget);
		current_widget->setPosition(coord);
		// update coord because of current_widget can have MinMax size
		coord = current_widget->getCoord();
		setPositionText(widgetContainer->position());

		UndoManager::getInstance().addValue(PR_POSITION);
	}
	current_widget_rectangle->setPosition(current_widget->getAbsoluteCoord());
}

void PropertiesPanelView::notifyRectangleKeyPressed(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	MyGUI::IntPoint delta;
	int k = MyGUI::InputManager::getInstance().isShiftPressed() ? 1 : grid_step;
	if (OIS::KC_TAB == _key)
	{
		if ((null != current_widget) && (null != current_widget->getParent()) && (current_widget->getParent()->getTypeName() == "Tab")) update(current_widget->getParent());
		if (current_widget->getTypeName() == "Tab")
		{
			MyGUI::TabPtr tab = current_widget->castType<MyGUI::Tab>();
			size_t sheet = tab->getItemIndexSelected();
			sheet++;
			if (sheet >= tab->getItemCount()) sheet = 0;
			if (tab->getItemCount()) tab->setItemSelectedAt(sheet);
		}
	}
	else if (OIS::KC_DELETE == _key)
	{
		if (current_widget){
			EditorWidgets::getInstance().remove(current_widget);
			eventRecreate(false);
			UndoManager::getInstance().addValue();
		}
	}
	else if (OIS::KC_LEFT == _key)
	{
		delta = MyGUI::IntPoint(-k, 0);
	}
	else if (OIS::KC_RIGHT == _key)
	{
		delta = MyGUI::IntPoint(k, 0);
	}
	else if (OIS::KC_UP == _key)
	{
		delta = MyGUI::IntPoint(0, -k);
	}
	else if (OIS::KC_DOWN == _key)
	{
		delta = MyGUI::IntPoint(0, k);
	}

	if (delta != MyGUI::IntPoint())
	{
		arrow_move = true;
		current_widget_rectangle->setPosition(current_widget_rectangle->getPosition() + delta);
		notifyRectangleResize(current_widget_rectangle);
		UndoManager::getInstance().addValue(PR_KEY_POSITION);
	}
}

void PropertiesPanelView::update(MyGUI::WidgetPtr _current_widget)
{
	current_widget = _current_widget;

	if (null == current_widget)
		current_widget_rectangle->hide();
	else
	{
		MyGUI::LayerManager::getInstance().upLayerItem(current_widget);
		MyGUI::IntCoord coord = current_widget->getCoord();
		MyGUI::WidgetPtr parent = current_widget->getParent();
		if (null != parent)
		{
			// ���� ������� ������ �� ����, �� ������� ���� ����
			if (parent->getTypeName() == "Sheet")
			{
				MyGUI::TabPtr tab = parent->getParent()->castType<MyGUI::Tab>();
				MyGUI::SheetPtr sheet = parent->castType<MyGUI::Sheet>();
				tab->setItemSelected(sheet);
			}
			// ���� ������� ���� ����, �� ������� ���� ����
			if (current_widget->getTypeName() == "Sheet")
			{
				MyGUI::TabPtr tab = parent->castType<MyGUI::Tab>();
				MyGUI::SheetPtr sheet = current_widget->castType<MyGUI::Sheet>();
				tab->setItemSelected(sheet);
			}
			coord = current_widget->getAbsoluteCoord();
		}
		current_widget_rectangle->show();
		current_widget_rectangle->setPosition(coord);
		MyGUI::InputManager::getInstance().setKeyFocusWidget(current_widget_rectangle);
	}

	// delete all previous properties
	for (std::vector<MyGUI::StaticTextPtr>::iterator iter = propertiesText.begin(); iter != propertiesText.end(); ++iter)
		(*iter)->hide();
	for (MyGUI::VectorWidgetPtr::iterator iter = propertiesElement.begin(); iter != propertiesElement.end(); ++iter)
		(*iter)->hide();

	if (null == _current_widget)
	{
		mainWidget()->hide();
	}
	else
	{
		mainWidget()->show();

		pairs_counter = 0;
		mPanelMainProperties.update(_current_widget);
		mPanelTypeProperties.update(_current_widget, PanelProperties::TYPE_PROPERTIES);
		mPanelGeneralProperties.update(_current_widget, PanelProperties::WIDGET_PROPERTIES);
		mPanelEvents.update(_current_widget, PanelProperties::EVENTS);
		mPanelItems.update(_current_widget);
		mPanelUserData.update(_current_widget);
	}
}

void PropertiesPanelView::createPropertiesWidgetsPair(MyGUI::WidgetPtr _window, std::string _property, std::string _value, std::string _type,int y)
{
	pairs_counter++;
	int x1 = 0, x2 = 125;
	int w1 = 120;
	int w2 = _window->getWidth() - x2;
	const int h = PropertyItemHeight;

	if (_property == "Position")
	{
		x1 = 66;
		w1 = w1 - x1;
	}

	MyGUI::StaticTextPtr text;
	MyGUI::WidgetPtr editOrCombo;
	//int string_int_float; // 0 - string, 1 - int, 2 - float

	int widget_for_type;// 0 - Edit, 1 - Combo mode drop, 2 - ...
	std::string type_names[2] = {"Edit", "ComboBox"};
	if ("Name" == _type) widget_for_type = 0;
	else if ("Skin" == _type) widget_for_type = 1;
	else if ("Position" == _type) widget_for_type = 0;
	else if ("Layer" == _type) widget_for_type = 1;
	else if ("String" == _type) widget_for_type = 0;
	else if ("Align" == _type) widget_for_type = 1;
	// �� ������ ��������� FIXME
	else if ("1 int" == _type) widget_for_type = 0;
	else if ("2 int" == _type) widget_for_type = 0;
	else if ("4 int" == _type) widget_for_type = 0;
	else if ("1 float" == _type) widget_for_type = 0;
	else if ("2 float" == _type) widget_for_type = 0;
	// ���� ������� ����� FIXME
	else if ("Colour" == _type) widget_for_type = 0;//"Colour" ������ �� ������������
	else if ("MessageButton" == _type) widget_for_type = 1;
	else if ("FileName" == _type) widget_for_type = 0;
	else widget_for_type = 1;

	if ((propertiesText.size() < pairs_counter) || (propertiesText[pairs_counter-1]->getParent() != _window))
	{
		text = _window->createWidget<MyGUI::StaticText>("Editor_StaticText", x1, y, w1, h, MyGUI::Align::Default);
		text->setTextAlign(MyGUI::Align::Right);
		if (propertiesText.size() < pairs_counter)
		{
			propertiesText.push_back(text);
		}
		else
		{
			MyGUI::Gui::getInstance().destroyWidget(propertiesText[pairs_counter-1]);
			propertiesText[pairs_counter-1] = text;
		}
	}
	else
	{
		text = propertiesText[pairs_counter-1];
		text->show();
		text->setPosition(x1, y, w1, h);
	}
	std::string prop = _property;
	// trim widget name
	std::string::iterator iter = std::find(prop.begin(), prop.end(), '_');
	if (iter != prop.end()) prop.erase(prop.begin(), ++iter);
	text->setCaption(prop);

	if ((propertiesElement.size() < pairs_counter) || (propertiesElement[pairs_counter-1]->getParent() != _window) ||
		(type_names[widget_for_type] != propertiesElement[pairs_counter-1]->getTypeName()))
	{
		if (widget_for_type == 0)
		{
			editOrCombo = _window->createWidget<MyGUI::Edit>("Edit", x2, y, w2, h, MyGUI::Align::Top | MyGUI::Align::HStretch);
			editOrCombo->castType<MyGUI::Edit>()->eventEditTextChange = newDelegate (this, &PropertiesPanelView::notifyTryApplyProperties);
			editOrCombo->castType<MyGUI::Edit>()->eventEditSelectAccept = newDelegate (this, &PropertiesPanelView::notifyForceApplyProperties);
		}
		else if (widget_for_type == 1)
		{
			editOrCombo = _window->createWidget<MyGUI::ComboBox>("ComboBox", x2, y, w2, h, MyGUI::Align::Top | MyGUI::Align::HStretch);
			editOrCombo->castType<MyGUI::ComboBox>()->eventComboAccept = newDelegate (this, &PropertiesPanelView::notifyForceApplyProperties);

			editOrCombo->castType<MyGUI::ComboBox>()->setComboModeDrop(true);
		}

		if (propertiesElement.size() < pairs_counter)
		{
			propertiesElement.push_back(editOrCombo);
		}
		else
		{
			MyGUI::Gui::getInstance().destroyWidget(propertiesElement[pairs_counter-1]);
			propertiesElement[pairs_counter-1] = editOrCombo;
		}
	}
	else
	{
		editOrCombo = propertiesElement[pairs_counter-1];
		if (widget_for_type == 1) editOrCombo->castType<MyGUI::ComboBox>()->removeAllItems();
		editOrCombo->show();
		editOrCombo->setPosition(x2, y, w2, h);
	}

	// fill possible values
	if (widget_for_type == 1)
	{
		std::vector<std::string> values;
		if (_type == "Skin") values = WidgetTypes::getInstance().find(current_widget->getTypeName())->skin;
		else values = WidgetTypes::getInstance().findPossibleValues(_type);

		for (std::vector<std::string>::iterator iter = values.begin(); iter != values.end(); ++iter)
			editOrCombo->castType<MyGUI::ComboBox>()->addItem(*iter);
	}

	editOrCombo->setUserString("action", _property);
	editOrCombo->setUserString("type", _type);

	if (_value.empty()){
		editOrCombo->setCaption(DEFAULT_VALUE);
	}
	else
	{
		editOrCombo->castType<MyGUI::Edit>()->setOnlyText(_value);
		checkType(editOrCombo->castType<MyGUI::Edit>(), _type);
	}
}

bool PropertiesPanelView::checkType(MyGUI::EditPtr _edit, std::string _type)
{
	bool success = true;
	if ("Name" == _type)
	{
		const Ogre::UTFString & text = _edit->getOnlyText();
		size_t index = _edit->getTextCursor();
		WidgetContainer * textWC = EditorWidgets::getInstance().find(text);
		if ((!text.empty()) && (null != textWC) &&
			(EditorWidgets::getInstance().find(current_widget) != textWC))
		{
			static const Ogre::UTFString colour = "#FF0000";
			_edit->setCaption(colour + text);
			success = false;
		}
		else
		{
			_edit->setCaption(text);
			success = true;
		}
		_edit->setTextCursor(index);
	}
	//else if ("Skin" == _type) widget_for_type = 1;
	//else
	if ("Position" == _type){
		if (EditorWidgets::getInstance().find(current_widget)->relative_mode)
			success = Parse::checkParce<float>(_edit, 4);
		else
			success = Parse::checkParce<int>(_edit, 4);
	}
	//else if ("Layer" == _type) // ��������� �� ����� ������ ���������, � ���� � ������� ����� ���� - ���� ��������
	//else if ("String" == _type) // ������������ ������? O_o
	//else if ("Align" == _type) // ��������� �� ����� ������ ���������, � ���� � ������� ����� ���� - ���� ��������
	else if ("1 int" == _type) success = Parse::checkParce<int>(_edit, 1);
	else if ("2 int" == _type) success = Parse::checkParce<int>(_edit, 2);
	else if ("4 int" == _type) success = Parse::checkParce<int>(_edit, 4);
	else if ("1 float" == _type) success = Parse::checkParce<float>(_edit, 1);
	else if ("2 float" == _type) success = Parse::checkParce<float>(_edit, 2);
	// ���� ������� ������������ � ��� �������� FIXME
	//else if ("Colour" == _type) success = Parse::checkParce<float>(_edit, 4);
	else if ("FileName" == _type) success = Parse::checkParceFileName(_edit);
	return success;
}

void PropertiesPanelView::notifyApplyProperties(MyGUI::WidgetPtr _sender, bool _force)
{
	EditorWidgets * ew = &EditorWidgets::getInstance();
	WidgetContainer * widgetContainer = ew->find(current_widget);
	MyGUI::EditPtr senderEdit = _sender->castType<MyGUI::Edit>();
	std::string action = senderEdit->getUserString("action");
	std::string value = senderEdit->getOnlyText();
	std::string type = senderEdit->getUserString("type");

	ON_EXIT(UndoManager::getInstance().addValue(PR_PROPERTIES););

	bool goodData = checkType(senderEdit, type);

	if (value == "[DEFAULT]" && senderEdit->getCaption() == DEFAULT_VALUE) value = "";

	if (action == "Name")
	{
		if (goodData)
		{
			widgetContainer->name = value;
			ew->widgets_changed = true;
		}
		return;
	}
	else if (action == "Skin")
	{
		widgetContainer->skin = value;
		if ( MyGUI::SkinManager::getInstance().isExist(widgetContainer->skin) )
		{
			MyGUI::xml::xmlDocument * save = ew->savexmlDocument();
			ew->clear();
			ew->loadxmlDocument(save);
			delete save;
			eventRecreate(false);
		}
		else
		{
			std::string mess = MyGUI::utility::toString("Skin '", widgetContainer->skin, "' not found. This value will be saved.");
			MyGUI::Message::_createMessage("Error", mess , "", "Overlapped", true, null, MyGUI::Message::IconError | MyGUI::Message::Ok);
		}
		return;
	}
	else if (action == "Position")
	{
		if (!goodData) return;
		if (widgetContainer->relative_mode){
			std::istringstream str(value);
			MyGUI::FloatCoord float_coord;
			str >> float_coord;
			float_coord.left = float_coord.left/100;
			float_coord.top = float_coord.top/100;
			float_coord.width = float_coord.width/100;
			float_coord.height = float_coord.height/100;
			MyGUI::IntCoord coord = MyGUI::WidgetManager::getInstance().convertRelativeToInt(float_coord, current_widget->getParent());
			current_widget->setPosition(coord);
			current_widget_rectangle->setPosition(current_widget->getAbsoluteCoord());
			return;
		}
		MyGUI::WidgetManager::getInstance().parse(widgetContainer->widget, "Widget_Move", value);
		current_widget_rectangle->setPosition(current_widget->getAbsoluteCoord());
		return;
	}
	else if (action == "Align")
	{
		widgetContainer->align = value;
		widgetContainer->widget->setAlign(MyGUI::Align::parse(value));
		return;
	}
	else if (action == "Layer")
	{
		widgetContainer->layer = value;
		return;
	}

	bool success = false;
	if (goodData || _force)
		success = ew->tryToApplyProperty(widgetContainer->widget, action, value);
	else
		return;

	if (success)
	{
		current_widget_rectangle->setPosition(current_widget->getAbsoluteCoord());
	}
	else
	{
		senderEdit->setCaption(DEFAULT_VALUE);
		return;
	}

	// ���� ����� ��-�� ����, �� ������� (��� ������ ���� ������) ��������
	for (StringPairs::iterator iterProperty = widgetContainer->mProperty.begin(); iterProperty != widgetContainer->mProperty.end(); ++iterProperty)
	{
		if (iterProperty->first == action){
			if (value.empty()) widgetContainer->mProperty.erase(iterProperty);
			else iterProperty->second = value;
			return;
		}
	}

	// ���� ������ �������� �� ���� ������, �� ���������
	widgetContainer->mProperty.push_back(std::make_pair(action, value));
}

void PropertiesPanelView::notifyTryApplyProperties(MyGUI::WidgetPtr _sender)
{
	notifyApplyProperties(_sender, false);
}

void PropertiesPanelView::notifyForceApplyProperties(MyGUI::WidgetPtr _sender)
{
	notifyApplyProperties(_sender, true);
}
