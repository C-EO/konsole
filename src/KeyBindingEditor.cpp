/*
    SPDX-FileCopyrightText: 2008 Robert Knight <robertknight@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
// Own
#include "KeyBindingEditor.h"

// Qt
#include <QDialogButtonBox>
#include <QIcon>
#include <QKeyEvent>

// KDE
#include <KLocalizedString>
#include <KMessageBox>

// Konsole
#include "keyboardtranslator/KeyboardTranslator.h"
#include "keyboardtranslator/KeyboardTranslatorManager.h"
#include "keyboardtranslator/KeyboardTranslatorReader.h"
#include "ui_KeyBindingEditor.h"

#include "widgets/EditProfileDialog.h"

using namespace Konsole;

KeyBindingEditor::KeyBindingEditor(QWidget *parent)
    : QDialog(parent)
    , _ui(nullptr)
    , _translator(new KeyboardTranslator(QString()))
    , _isNewTranslator(false)
{
    auto layout = new QVBoxLayout;

    auto mainWidget = new QWidget(this);
    layout->addWidget(mainWidget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
    layout->addWidget(buttonBox);

    setLayout(layout);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &Konsole::KeyBindingEditor::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setAttribute(Qt::WA_DeleteOnClose);

    _ui = new Ui::KeyBindingEditor();
    _ui->setupUi(mainWidget);

    // description edit
    _ui->descriptionEdit->setPlaceholderText(i18nc("@label:textbox", "Enter descriptive label"));
    connect(_ui->descriptionEdit, &QLineEdit::textChanged, this, &Konsole::KeyBindingEditor::setTranslatorDescription);
    // filter edit
    connect(_ui->filterEdit, &QLineEdit::textChanged, this, &Konsole::KeyBindingEditor::filterRows);

    // key bindings table
    _ui->keyBindingTable->setColumnCount(2);

    QStringList labels;
    labels << i18n("Key Combination") << i18n("Output");

    _ui->keyBindingTable->setHorizontalHeaderLabels(labels);
    _ui->keyBindingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    _ui->keyBindingTable->verticalHeader()->hide();
    _ui->keyBindingTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // add and remove buttons
    _ui->addEntryButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    _ui->removeEntryButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));

    connect(_ui->removeEntryButton, &QPushButton::clicked, this, &Konsole::KeyBindingEditor::removeSelectedEntry);
    connect(_ui->addEntryButton, &QPushButton::clicked, this, &Konsole::KeyBindingEditor::addNewEntry);

    // test area
    _ui->testAreaInputEdit->installEventFilter(this);
}

KeyBindingEditor::~KeyBindingEditor()
{
    delete _ui;
    delete _translator;
}

void KeyBindingEditor::filterRows(const QString &text)
{
    const int rows = _ui->keyBindingTable->rowCount();

    QList<int> matchedRows;

    const auto items = _ui->keyBindingTable->findItems(text, Qt::MatchContains);
    for (QTableWidgetItem *matchedItem : items) {
        matchedRows.append(matchedItem->row());
    }

    for (int i = 0; i < rows; i++) {
        if (matchedRows.contains(i) && _ui->keyBindingTable->isRowHidden(i)) {
            _ui->keyBindingTable->showRow(i);
        } else if (!matchedRows.contains(i)) {
            _ui->keyBindingTable->hideRow(i);
        }
    }
}

void KeyBindingEditor::removeSelectedEntry()
{
    QList<QTableWidgetItem *> uniqueList;
    const QList<QTableWidgetItem *> selectedItems = _ui->keyBindingTable->selectedItems();
    for (QTableWidgetItem *item : selectedItems) {
        if (item->column() == 1) { // Select item at the first column
            item = _ui->keyBindingTable->item(item->row(), 0);
        }

        if (!uniqueList.contains(item)) {
            uniqueList.append(item);
        }
    }

    for (QTableWidgetItem *item : std::as_const(uniqueList)) {
        // get the first item in the row which has the entry

        KeyboardTranslator::Entry existing = item->data(Qt::UserRole).value<KeyboardTranslator::Entry>();

        _translator->removeEntry(existing);

        _ui->keyBindingTable->removeRow(item->row());
    }
}

void KeyBindingEditor::addNewEntry()
{
    _ui->keyBindingTable->insertRow(_ui->keyBindingTable->rowCount());

    int newRowCount = _ui->keyBindingTable->rowCount();

    // block signals here to avoid triggering bindingTableItemChanged() slot call
    _ui->keyBindingTable->blockSignals(true);

    _ui->keyBindingTable->setItem(newRowCount - 1, 0, new QTableWidgetItem());
    _ui->keyBindingTable->setItem(newRowCount - 1, 1, new QTableWidgetItem());

    _ui->keyBindingTable->blockSignals(false);

    // make sure user can see new row
    _ui->keyBindingTable->scrollToItem(_ui->keyBindingTable->item(newRowCount - 1, 0));
}

bool KeyBindingEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == _ui->testAreaInputEdit) {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);

            // The state here is currently set to the state that a newly started
            // terminal in Konsole will be in ( which is also the same as the
            // state just after a reset ), this has 'ANSI' turned on and all other
            // states off.
            //
            // TODO: It may be useful to be able to specify the state in the 'test input'
            // area, but preferably not in a way which clutters the UI with lots of
            // checkboxes.
            //
            const KeyboardTranslator::States states = KeyboardTranslator::AnsiState;

            KeyboardTranslator::Entry entry = _translator->findEntry(keyEvent->key(), keyEvent->modifiers(), states);

            if (!entry.isNull()) {
                _ui->testAreaInputEdit->setText(entry.conditionToString());
                _ui->testAreaOutputEdit->setText(entry.resultToString(true, keyEvent->modifiers()));
            } else {
                _ui->testAreaInputEdit->setText(keyEvent->text());
                _ui->testAreaOutputEdit->setText(keyEvent->text());
            }

            keyEvent->accept();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void KeyBindingEditor::setDescription(const QString &description)
{
    _ui->descriptionEdit->setText(description);

    setTranslatorDescription(description);
}

void KeyBindingEditor::setTranslatorDescription(const QString &description)
{
    if (_translator != nullptr) {
        _translator->setDescription(description);
    }
}

QString KeyBindingEditor::description() const
{
    return _ui->descriptionEdit->text();
}

void KeyBindingEditor::setup(const KeyboardTranslator *translator, const QString &currentProfileTranslator, bool isNewTranslator)
{
    delete _translator;

    _isNewTranslator = isNewTranslator;

    _currentProfileTranslator = currentProfileTranslator;

    _translator = new KeyboardTranslator(*translator);

    // setup description edit line
    _ui->descriptionEdit->setClearButtonEnabled(true);
    // setup filter edit line
    _ui->filterEdit->setClearButtonEnabled(true);

    if (_isNewTranslator) {
        setDescription(i18n("New Key Binding List"));
        setWindowTitle(i18n("New Key Binding List"));
    } else {
        _ui->descriptionEdit->setText(translator->description());
        setWindowTitle(i18n("Edit Key Binding List"));
    }

    // setup key binding table
    setupKeyBindingTable(translator);
}

KeyboardTranslator *KeyBindingEditor::translator() const
{
    return _translator;
}

void KeyBindingEditor::bindingTableItemChanged(QTableWidgetItem *item)
{
    QTableWidgetItem *key = _ui->keyBindingTable->item(item->row(), 0);
    KeyboardTranslator::Entry existing = key->data(Qt::UserRole).value<KeyboardTranslator::Entry>();

    QString condition = key->text();
    QString result = _ui->keyBindingTable->item(item->row(), 1)->text();

    KeyboardTranslator::Entry entry = KeyboardTranslatorReader::createEntry(condition, result);
    _translator->replaceEntry(existing, entry);

    // block signals to prevent this slot from being called repeatedly
    _ui->keyBindingTable->blockSignals(true);

    key->setData(Qt::UserRole, QVariant::fromValue(entry));

    _ui->keyBindingTable->blockSignals(false);
}

void KeyBindingEditor::setupKeyBindingTable(const KeyboardTranslator *translator)
{
    disconnect(_ui->keyBindingTable, &QTableWidget::itemChanged, this, &Konsole::KeyBindingEditor::bindingTableItemChanged);

    QList<KeyboardTranslator::Entry> entries = translator->entries();
    _ui->keyBindingTable->setRowCount(entries.count());

    for (int row = 0; row < entries.count(); row++) {
        const KeyboardTranslator::Entry &entry = entries.at(row);

        QTableWidgetItem *keyItem = new QTableWidgetItem(entry.conditionToString());
        keyItem->setData(Qt::UserRole, QVariant::fromValue(entry));

        QTableWidgetItem *textItem = new QTableWidgetItem(entry.resultToString());

        _ui->keyBindingTable->setItem(row, 0, keyItem);
        _ui->keyBindingTable->setItem(row, 1, textItem);
    }
    _ui->keyBindingTable->sortItems(0);

    connect(_ui->keyBindingTable, &QTableWidget::itemChanged, this, &Konsole::KeyBindingEditor::bindingTableItemChanged);
}

void KeyBindingEditor::accept()
{
    if (_translator == nullptr) {
        return;
    }

    const auto newTranslator = new KeyboardTranslator(*_translator);

    if (newTranslator->description().isEmpty()) {
        KMessageBox::error(this, i18n("A key bindings scheme cannot be saved with an empty description."));
        delete newTranslator;
        return;
    }

    if (_isNewTranslator) {
        newTranslator->setName(newTranslator->description());
    }

    KeyboardTranslatorManager::instance()->addTranslator(newTranslator);

    const QString &currentTranslatorName = newTranslator->name();

    Q_EMIT updateKeyBindingsListRequest(currentTranslatorName);

    if (currentTranslatorName == _currentProfileTranslator) {
        Q_EMIT updateTempProfileKeyBindingsRequest(Profile::KeyBindings, currentTranslatorName);
    }

    QDialog::accept();
}

QSize KeyBindingEditor::sizeHint() const
{
    const auto parent = parentWidget();
    if (parent != nullptr) {
        return {static_cast<int>(parent->width() * 0.9), static_cast<int>(parent->height() * 0.95)};
    }

    return {};
}

#include "moc_KeyBindingEditor.cpp"
