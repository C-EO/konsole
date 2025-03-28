/*
    SPDX-FileCopyrightText: 2007-2008 Robert Knight <robertknight@gmail.com>
    SPDX-FileCopyrightText: 1997, 1998 Lars Doelle <lars.doelle@on-line.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SCREEN_H
#define SCREEN_H

// STD
#include <memory>

// Qt
#include <QBitArray>
#include <QRect>
#include <QSet>
#include <QVarLengthArray>
#include <QVector>

// Konsole
#include "../characters/Character.h"
#include "konsoleprivate_export.h"

#define MODE_Origin 0
#define MODE_Wrap 1
#define MODE_Insert 2
#define MODE_Screen 3
#define MODE_Cursor 4
#define MODE_NewLine 5
#define MODE_AppScreen 6
#define MODE_SelectCursor 7
#define MODES_SCREEN 8

#define REPL_None 0
#define REPL_PROMPT 1
#define REPL_INPUT 2
#define REPL_OUTPUT 3

struct TerminalGraphicsPlacement_t {
    QPixmap pixmap;
    qint64 id;
    qint64 pid;
    int z, X, Y, col, row, cols, rows;
    qreal opacity;
    bool scrolling;
    enum source { Sixel, iTerm, Kitty } source;
};

namespace Konsole
{
class TerminalCharacterDecoder;
class TerminalDisplay;
class HistoryType;
class HistoryScroll;
class EscapeSequenceUrlExtractor;

/**
    \brief An image of characters with associated attributes.

    The terminal emulation ( Emulation ) receives a serial stream of
    characters from the program currently running in the terminal.
    From this stream it creates an image of characters which is ultimately
    rendered by the display widget ( TerminalDisplay ).  Some types of emulation
    may have more than one screen image.

    getImage() is used to retrieve the currently visible image
    which is then used by the display widget to draw the output from the
    terminal.

    The number of lines of output history which are kept in addition to the current
    screen image depends on the history scroll being used to store the output.
    The scroll is specified using setScroll()
    The output history can be retrieved using writeToStream()

    The screen image has a selection associated with it, specified using
    setSelectionStart() and setSelectionEnd().  The selected text can be retrieved
    using selectedText().  When getImage() is used to retrieve the visible image,
    characters which are part of the selection have their colors inverted.
*/
class KONSOLEPRIVATE_EXPORT Screen
{
public:
    /* PlainText: Return plain text (default)
     * ConvertToHtml: Specifies if returned text should have HTML tags.
     * PreserveLineBreaks: Specifies whether new line characters should be
     *      inserted into the returned text at the end of each terminal line.
     * TrimLeadingWhitespace: Specifies whether leading spaces should be
     *      trimmed in the returned text.
     * TrimTrailingWhitespace: Specifies whether trailing spaces should be
     *      trimmed in the returned text.
     */
    enum DecodingOption {
        PlainText = 0x0,
        ConvertToHtml = 0x1,
        PreserveLineBreaks = 0x2,
        TrimLeadingWhitespace = 0x4,
        TrimTrailingWhitespace = 0x8,
        ExcludePrompt = 0x10,
        ExcludeInput = 0x20,
        ExcludeOutput = 0x40,
    };
    Q_DECLARE_FLAGS(DecodingOptions, DecodingOption)

    /** Construct a new screen image of size @p lines by @p columns. */
    Screen(int lines, int columns);
    ~Screen();

    Screen(const Screen &) = delete;
    Screen &operator=(const Screen &) = delete;

    EscapeSequenceUrlExtractor *urlExtractor() const;

    // VT100/2 Operations
    // Cursor Movement

    /**
     * Move the cursor up by @p n lines.  The cursor will stop at the
     * top margin.
     */
    void cursorUp(int n);
    /**
     * Move the cursor down by @p n lines.  The cursor will stop at the
     * bottom margin.
     */
    void cursorDown(int n);
    /**
     * Move the cursor to the left by @p n columns.
     * The cursor will stop at the first column.
     */
    void cursorLeft(int n);
    /**
     * Moves cursor to beginning of the line by @p n lines down.
     * The cursor will stop at the beginning of the line.
     */
    void cursorNextLine(int n);
    /**
     * Moves cursor to beginning of the line by @p n lines up.
     * The cursor will stop at the beginning of the line.
     */
    void cursorPreviousLine(int n);
    /**
     * Move the cursor to the right by @p n columns.
     * The cursor will stop at the right-most column.
     */
    void cursorRight(int n);
    /** Position the cursor on line @p y. */
    void setCursorY(int y);
    /** Position the cursor at column @p x. */
    void setCursorX(int x);
    /** Position the cursor at line @p y, column @p x. */
    void setCursorYX(int y, int x);

    void initSelCursor();
    int selCursorUp(int n);
    int selCursorDown(int n);
    int selCursorLeft(int n);
    int selCursorRight(int n);

    int selSetSelectionStart(int mode);
    int selSetSelectionEnd(int mode);

    /**
     * Sets the margins for scrolling the screen.
     *
     * @param top The top line of the new scrolling margin.
     * @param bot The bottom line of the new scrolling margin.
     */
    void setMargins(int top, int bot);
    /** Returns the top line of the scrolling region. */
    int topMargin() const;
    /** Returns the bottom line of the scrolling region. */
    int bottomMargin() const;

    /**
     * Resets the scrolling margins back to the top and bottom lines
     * of the screen.
     */
    void setDefaultMargins();

    /**
     * Moves the cursor down one line, if the MODE_NewLine mode
     * flag is enabled then the cursor is returned to the leftmost
     * column first.
     *
     * Equivalent to NextLine() if the MODE_NewLine flag is set
     * or index() otherwise.
     */
    void newLine();
    /**
     * Moves the cursor down one line and positions it at the beginning
     * of the line.  Equivalent to calling Return() followed by index()
     */
    void nextLine();

    /**
     * Move the cursor down one line.  If the cursor is on the bottom
     * line of the scrolling region (as returned by bottomMargin()) the
     * scrolling region is scrolled up by one line instead.
     */
    void index();
    /**
     * Move the cursor up one line.  If the cursor is on the top line
     * of the scrolling region (as returned by topMargin()) the scrolling
     * region is scrolled down by one line instead.
     */
    void reverseIndex();

    /**
     * Scroll the scrolling region of the screen up by @p n lines.
     * The scrolling region is initially the whole screen, but can be changed
     * using setMargins()
     */
    void scrollUp(int n);
    /**
     * Scroll the scrolling region of the screen down by @p n lines.
     * The scrolling region is initially the whole screen, but can be changed
     * using setMargins()
     */
    void scrollDown(int n);
    /**
     * Moves the cursor to the beginning of the current line.
     * Equivalent to setCursorX(0)
     */
    void toStartOfLine();
    /**
     * Moves the cursor one column to the left and erases the character
     * at the new cursor position.
     */
    void backspace();
    /** Moves the cursor @p n tab-stops to the right. */
    void tab(int n = 1);
    /** Moves the cursor @p n tab-stops to the left. */
    void backtab(int n);

    // Editing

    /**
     * Erase @p n characters beginning from the current cursor position.
     * This is equivalent to over-writing @p n characters starting with the current
     * cursor position with spaces.
     * If @p n is 0 then one character is erased.
     */
    void eraseChars(int n);
    /**
     * Delete @p n characters beginning from the current cursor position.
     * If @p n is 0 then one character is deleted.
     */
    void deleteChars(int n);
    /**
     * Insert @p n blank characters beginning from the current cursor position.
     * The position of the cursor is not altered.
     * If @p n is 0 then one character is inserted.
     */
    void insertChars(int n);
    /**
     * Repeat the preceding graphic character @p n times, including SPACE.
     * If @p n is 0 then the character is repeated once.
     */
    void repeatChars(int n);
    /**
     * Removes @p n lines beginning from the current cursor position.
     * The position of the cursor is not altered.
     * If @p n is 0 then one line is removed.
     */
    void deleteLines(int n);
    /**
     * Inserts @p lines beginning from the current cursor position.
     * The position of the cursor is not altered.
     * If @p n is 0 then one line is inserted.
     */
    void insertLines(int n);
    /** Clears all the tab stops. */
    void clearTabStops();
    /**  Sets or removes a tab stop at the cursor's current column. */
    void changeTabStop(bool set);

    /** Resets (clears) the specified screen @p m. */
    void resetMode(int m);
    /** Sets (enables) the specified screen @p m. */
    void setMode(int m);
    /**
     * Saves the state of the specified screen @p m.  It can be restored
     * using restoreMode()
     */
    void saveMode(int m);
    /** Restores the state of a screen @p m saved by calling saveMode() */
    void restoreMode(int m);
    /** Returns whether the specified screen @p me is enabled or not .*/
    bool getMode(int m) const;

    /**
     * Saves the current position and appearance (text color and style) of the cursor.
     * It can be restored by calling restoreCursor()
     */
    void saveCursor();
    /** Restores the position and appearance of the cursor.  See saveCursor() */
    void restoreCursor();

    /** Clear the whole screen, moving the current screen contents into the history first. */
    void clearEntireScreen();
    /**
     * Clear the area of the screen from the current cursor position to the end of
     * the screen.
     */
    void clearToEndOfScreen();
    /**
     * Clear the area of the screen from the current cursor position to the start
     * of the screen.
     */
    void clearToBeginOfScreen();
    /** Clears the whole of the line on which the cursor is currently positioned. */
    void clearEntireLine();
    /** Clears from the current cursor position to the end of the line. */
    void clearToEndOfLine();
    /** Clears from the current cursor position to the beginning of the line. */
    void clearToBeginOfLine();

    /** Fills the entire screen with the letter 'E' */
    void helpAlign();

    /**
     * Enables the given @p rendition flag.  Rendition flags control the appearance
     * of characters on the screen.
     *
     * @see Character::rendition
     */
    void setRendition(RenditionFlags rendition);
    void setUnderlineType(int type);
    /**
     * Disables the given @p rendition flag.  Rendition flags control the appearance
     * of characters on the screen.
     *
     * @see Character::rendition
     */
    void resetRendition(RenditionFlags rendition);

    /**
     * Sets the cursor's foreground color.
     * @param space The color space used by the @p color argument
     * @param color The new foreground color.  The meaning of this depends on
     * the color @p space used.
     *
     * @see CharacterColor
     */
    void setForeColor(int space, int color);
    /**
     * Sets the cursor's background color.
     * @param space The color space used by the @p color argument.
     * @param color The new background color.  The meaning of this depends on
     * the color @p space used.
     *
     * @see CharacterColor
     */
    void setBackColor(int space, int color);
    /**
     * Resets the cursor's color back to the default and sets the
     * character's rendition flags back to the default settings.
     */
    void setDefaultRendition();

    void setULColor(int space, int color);

    CharacterColor const *ulColorTable() const
    {
        return _ulColors;
    }

    /** Returns the column which the cursor is positioned at. */
    int getCursorX() const;
    /** Returns the line which the cursor is positioned on. */
    int getCursorY() const;

    /**
     * Resets the state of the screen.  This resets the various screen modes
     * back to their default states.  The cursor style and colors are reset
     * (as if setDefaultRendition() had been called)
     *
     * <ul>
     * <li>Line wrapping is enabled.</li>
     * <li>Origin mode is disabled.</li>
     * <li>Insert mode is disabled.</li>
     * <li>Cursor mode is enabled.  TODO Document me</li>
     * <li>Screen mode is disabled. TODO Document me</li>
     * <li>New line mode is disabled.  TODO Document me</li>
     * </ul>
     *
     * If @p softReset is true then perform a DECSTR,
     * otherwise perform RIS (Reset to Initial State).
     *
     * If @p preservePrompt is true, then attempt to preserve the
     * line with the command prompt even on a RIS.
     */
    void reset(bool softReset = false, bool preservePrompt = false);

    /**
     * Displays a new character at the current cursor position.
     *
     * If the cursor is currently positioned at the right-edge of the screen and
     * line wrapping is enabled then the character is added at the start of a new
     * line below the current one.
     *
     * If the MODE_Insert screen mode is currently enabled then the character
     * is inserted at the current cursor position, otherwise it will replace the
     * character already at the current cursor position.
     */
    void displayCharacter(uint c);

    /**
     * Resizes the image to a new fixed size of @p new_lines by @p new_columns.
     * In the case that @p new_columns is smaller than the current number of columns,
     * existing lines are not truncated.  This prevents characters from being lost
     * if the terminal display is resized smaller and then larger again.
     *
     * The top and bottom margins are reset to the top and bottom of the new
     * screen size.  Tab stops are also reset and the current selection is
     * cleared.
     */
    void resizeImage(int new_lines, int new_columns);

    /**
     * Returns the current screen image.
     * The result is an array of Characters of size [getLines()][getColumns()] which
     * must be freed by the caller after use.
     *
     * @param dest Buffer to copy the characters into
     * @param size Size of @p dest in Characters
     * @param startLine Index of first line to copy
     * @param endLine Index of last line to copy
     */
    void getImage(Character *dest, int size, int startLine, int endLine) const;

    /**
     * Returns the additional attributes associated with lines in the image.
     * The most important attribute is LINE_WRAPPED which specifies that the
     * line is wrapped,
     * other attributes control the size of characters in the line.
     */
    QVector<LineProperty> getLineProperties(int startLine, int endLine) const;

    /** Return the number of lines. */
    int getLines() const
    {
        return _lines;
    }

    /** Return the number of columns. */
    int getColumns() const
    {
        return _columns;
    }

    /** Return the number of lines in the history buffer. */
    int getHistLines() const;
    /**
     * Sets the type of storage used to keep lines in the history.
     * If @p copyPreviousScroll is true then the contents of the previous
     * history buffer are copied into the new scroll.
     */
    void setScroll(const HistoryType &, bool copyPreviousScroll = true);
    /** Returns the type of storage used to keep lines in the history. */
    const HistoryType &getScroll() const;
    /**
     * Returns true if this screen keeps lines that are scrolled off the screen
     * in a history buffer.
     */
    bool hasScroll() const;

    /**
     * Sets the start of the selection.
     *
     * @param x The column index of the first character in the selection.
     * @param y The line index of the first character in the selection.
     * @param blockSelectionMode True if the selection is in column mode.
     */
    void setSelectionStart(const int x, const int y, const bool blockSelectionMode);

    /**
     * Sets the end of the current selection.
     *
     * @param x The column index of the last character in the selection.
     * @param y The line index of the last character in the selection.
     * @param trimTrailingWhitespace True if trailing whitespace is trimmed from the selection
     */
    void setSelectionEnd(const int x, const int y, const bool trimTrailingWhitespace);

    /**
     * Selects a range of characters with the same REPL mode as the character at (@p x, @p y)
     */
    void selectReplContigious(const int x, const int y);

    /**
     * Retrieves the start of the selection or the cursor position if there
     * is no selection.
     */
    void getSelectionStart(int &column, int &line) const;

    /**
     * Retrieves the end of the selection or the cursor position if there
     * is no selection.
     */
    void getSelectionEnd(int &column, int &line) const;

    /** Clears the current selection */
    void clearSelection();

    /** Return the selection state */
    bool hasSelection() const;

    /**
     *  Returns true if the character at (@p x, @p y) is part of the
     *  current selection.
     */
    bool isSelected(const int x, const int y) const;

    /**
     * Convenience method.  Returns the currently selected text.
     * @param options See Screen::DecodingOptions
     */
    QString selectedText(const DecodingOptions options) const;

    /**
     * Convenience method.  Returns the text between two indices.
     * @param startIndex Specifies the starting text index
     * @param endIndex Specifies the ending text index
     * @param options See Screen::DecodingOptions
     */
    QString text(int startIndex, int endIndex, const DecodingOptions options) const;

    /**
     * Copies part of the output to a stream.
     *
     * @param decoder A decoder which converts terminal characters into text
     * @param fromLine The first line in the history to retrieve
     * @param toLine The last line in the history to retrieve
     */
    void writeLinesToStream(TerminalCharacterDecoder *decoder, int fromLine, int toLine) const;

    /**
     * Checks if the text between from and to is inside the current
     * selection. If this is the case, the selection is cleared. The
     * from and to are coordinates in the current viewable window.
     * The loc(x,y) macro can be used to generate these values from a
     * column,line pair.
     *
     * @param from The start of the area to check.
     * @param to The end of the area to check
     */
    void checkSelection(int from, int to);

    /**
     * Sets or clears an attribute of the current line.
     *
     * @param property The attribute to set or clear
     * Possible properties are:
     * LINE_WRAPPED:     Specifies that the line is wrapped.
     * LINE_DOUBLEWIDTH: Specifies that the characters in the current line
     *                   should be double the normal width.
     * LINE_DOUBLEHEIGHT_TOP:
     * LINE_DOUBLEHEIGHT_BOTTOM: Specifies that the characters in the current line
     *                   should be double the normal height.
     *                   Double-height lines are formed of two lines containing the same characters,
     *                   with the top one having the LINE_DOUBLEHEIGHT_TOP attribute
     *                   and the bottom one having the LINE_DOUBLEHEIGHT_BOTTOM attribute.
     *                   This allows other parts of the code to work on the
     *                   assumption that all lines are the same height.
     *
     * @param enable true to apply the attribute to the current line or false to remove it
     */
    void setLineProperty(quint16 property, bool enable);

    /**
     * Set REPL mode (shell integration)
     *
     * @param mode is the REPL mode
     * Possible modes are:
     * REPL_None
     * REPL_PROMPT
     * REPL_INPUT
     * REPL_OUTPUT
     */
    void setReplMode(int mode);
    void setExitCode(int exitCode);
    /** Return true if semantic shell integration is in use. */
    bool hasRepl() const
    {
        return _hasRepl;
    }
    /** Return current REPL mode */
    int replMode() const
    {
        return _replMode;
    }
    /** Return location of current REPL mode start. */
    std::pair<int, int> replModeStart() const
    {
        return _replModeStart;
    }
    std::pair<int, int> replModeEnd() const
    {
        return _replModeEnd;
    }

    /**
     * Returns the number of lines that the image has been scrolled up or down by,
     * since the last call to resetScrolledLines().
     *
     * a positive return value indicates that the image has been scrolled up,
     * a negative return value indicates that the image has been scrolled down.
     */
    int scrolledLines() const;

    /**
     * Returns the region of the image which was last scrolled.
     *
     * This is the area of the image from the top margin to the
     * bottom margin when the last scroll occurred.
     */
    QRect lastScrolledRegion() const;

    /**
     * Resets the count of the number of lines that the image has been scrolled up or down by,
     * see scrolledLines()
     */
    void resetScrolledLines();

    /**
     * Returns the number of lines of output which have been
     * dropped from the history since the last call
     * to resetDroppedLines()
     *
     * If the history is not unlimited then it will drop
     * the oldest lines of output if new lines are added when
     * it is full.
     */
    int droppedLines() const;

    int fastDroppedLines() const;

    /**
     * Resets the count of the number of lines dropped from
     * the history.
     */
    void resetDroppedLines();

    /**
     * Fills the buffer @p dest with @p count instances of the default (ie. blank)
     * Character style.
     */
    static void fillWithDefaultChar(Character *dest, int count);

    void setCurrentTerminalDisplay(TerminalDisplay *display)
    {
        _currentTerminalDisplay = display;
    }

    TerminalDisplay *currentTerminalDisplay()
    {
        return _currentTerminalDisplay;
    }

    QSet<uint> usedExtendedChars() const
    {
        QSet<uint> result;
        for (int i = 0; i < _lines; ++i) {
            const ImageLine &il = _screenLines[i];
            for (int j = 0; j < il.length(); ++j) {
                if (il[j].rendition.f.extended) {
                    result << il[j].character;
                }
            }
        }
        return result;
    }

    void setEnableUrlExtractor(const bool enable);

    static const Character DefaultChar;
    static const Character VisibleChar;

    // Return the total number of lines before resize (fix scroll glitch)
    int getOldTotalLines();
    // Return if it was a resize signal (fix scroll glitch)
    bool isResize();
    // Set reflow condition
    void setReflowLines(bool enable);

    /* Graphics display functions */
    void addPlacement(QPixmap pixmap,
                      int &rows,
                      int &cols,
                      int row = -1,
                      int col = -1,
                      enum TerminalGraphicsPlacement_t::source source = TerminalGraphicsPlacement_t::Sixel,
                      bool scrolling = true,
                      int moveCursor = 1,
                      bool leaveText = false,
                      int z = -1000,
                      int id = -1,
                      int pid = -1,
                      qreal opacity = 1.0,
                      int X = 0,
                      int Y = 0);
    TerminalGraphicsPlacement_t *getGraphicsPlacement(unsigned int i);
    void delPlacements(int = 'a', qint64 = 0, qint64 = -1, int = 0, int = 0, int = 0);

    bool hasGraphics() const
    {
        return _hasGraphics;
    }
    void setIgnoreWcWidth(bool ignore);

    QList<int> getCharacterCounts() const;

private:
    // copies a line of text from the screen or history into a stream using a
    // specified character decoder.  Returns the number of lines actually copied,
    // which may be less than 'count' if (start+count) is more than the number of characters on
    // the line
    //
    // line - the line number to copy, from 0 (the earliest line in the history) up to
    //         history->getLines() + lines - 1
    // start - the first column on the line to copy
    // count - the number of characters on the line to copy
    // decoder - a decoder which converts terminal characters (an Character array) into text
    // appendNewLine - if true a new line character (\n) is appended to the end of the line
    // isBlockSelectionMode - takes that in consideration while appending a new line.
    int copyLineToStream(int line,
                         int start,
                         int count,
                         TerminalCharacterDecoder *decoder,
                         bool appendNewLine,
                         bool isBlockSelectionMode,
                         const DecodingOptions options) const;

    // fills a section of the screen image with the character 'c'
    // the parameters are specified as offsets from the start of the screen image.
    // the loc(x,y) macro can be used to generate these values from a column,line pair.
    // if resetLineRendition is true, all completely cleared lines will be set to single-width.
    void clearImage(int loca, int loce, char c, bool resetLineRendition = true);

    // erases a rectangular section of the screen.
    void eraseBlock(int y, int x, int height, int width);

    // move screen image between 'sourceBegin' and 'sourceEnd' to 'dest'.
    // the parameters are specified as offsets from the start of the screen image.
    // the loc(x,y) macro can be used to generate these values from a column,line pair.
    //
    // NOTE: moveImage() can only move whole lines
    void moveImage(int dest, int sourceBegin, int sourceEnd);
    // scroll up 'n' lines in current region, clearing the bottom 'n' lines
    void scrollUp(int from, int n);
    // scroll down 'n' lines in current region, clearing the top 'n' lines
    void scrollDown(int from, int n);

    // when we handle scroll commands, we need to know which screenwindow will scroll
    TerminalDisplay *_currentTerminalDisplay;

    void addHistLine();
    // add lines from _screen to _history and remove from _screen the added lines (used to resize lines and columns)
    void fastAddHistLine();

    void initTabStops();

    void updateEffectiveRendition();
    void reverseRendition(Character &p) const;

    bool isSelectionValid() const;
    // copies text from 'startIndex' to 'endIndex' to a stream
    // startIndex and endIndex are positions generated using the loc(x,y) macro
    void writeToStream(TerminalCharacterDecoder *decoder, int startIndex, int endIndex, const DecodingOptions options) const;
    // copies 'count' lines from the screen buffer into 'dest',
    // starting from 'startLine', where 0 is the first line in the screen buffer
    void copyFromScreen(Character *dest, int startLine, int count) const;
    // copies 'count' lines from the history buffer into 'dest',
    // starting from 'startLine', where 0 is the first line in the history
    void copyFromHistory(Character *dest, int startLine, int count) const;

    Character getCharacter(int col, int row) const;

    // returns a buffer that can hold at most 'count' characters,
    // where the number of reallocations and object reinitializations
    // should be as minimal as possible
    static Character *getCharacterBuffer(const int size);

    // Returns if its app mode or not
    inline bool isAppMode()
    {
        return _currentModes[MODE_AppScreen] == 1;
    }
    // Get the cursor line after checking if its app mode or not
    int getCursorLine();
    // Set the cursor line after checking if its app mode or not
    void setCursorLine(int newLine);

    int getLineLength(const int line) const;

    // returns the width in columns of the specified screen line,
    // taking DECDWL/DECDHL (double width/height modes) into account.
    int getScreenLineColumns(const int line) const;

    // screen image ----------------
    int _lines;
    int _columns;

    typedef QVector<Character> ImageLine; // [0..columns]
    std::vector<ImageLine> _screenLines; // [lines]
    int _screenLinesSize; // _screenLines.size()

    int _scrolledLines;
    QRect _lastScrolledRegion;

    int _droppedLines;
    int _fastDroppedLines;

    int _oldTotalLines;
    bool _isResize;
    bool _enableReflowLines;

    std::vector<LineProperty> _lineProperties;
    LineProperty linePropertiesAt(unsigned int line);

    // history buffer ---------------
    std::unique_ptr<HistoryScroll> _history;

    // cursor location
    int _cuX;
    int _cuY;

    // select mode cursor location
    int _selCuX{};
    int _selCuY{};

    // cursor color and rendition info
    CharacterColor _currentForeground;
    CharacterColor _currentBackground;
    RenditionFlagsC _currentRendition;

    CharacterColor _ulColors[15];
    int _ulColorQueueStart;
    int _ulColorQueueEnd;
    int _currentULColor;

    // margins ----------------
    int _topMargin;
    int _bottomMargin;

    // states ----------------
    int _currentModes[MODES_SCREEN];
    int _savedModes[MODES_SCREEN];
    int _replMode;
    bool _hasRepl;
    bool _replHadOutput;
    std::pair<int, int> _replModeStart;
    std::pair<int, int> _replModeEnd;
    std::pair<int, int> _replLastOutputStart;
    std::pair<int, int> _replLastOutputEnd;
    int commandCounter = 0;

    // ----------------------------

    QBitArray _tabStops;

    // selection -------------------
    int _selBegin; // The first location selected.
    int _selTopLeft; // TopLeft Location.
    int _selBottomRight; // Bottom Right Location.
    bool _blockSelectionMode; // Column selection mode

    // effective colors and rendition ------------
    CharacterColor _effectiveForeground; // These are derived from
    CharacterColor _effectiveBackground; // the cu_* variables above
    RenditionFlagsC _effectiveRendition; // to speed up operation

    class SavedState
    {
    public:
        SavedState()
            : cursorColumn(0)
            , cursorLine(0)
            , originMode(0)
            , rendition({0})
            , foreground(CharacterColor())
            , background(CharacterColor())
        {
        }

        int cursorColumn;
        int cursorLine;
        int originMode;
        RenditionFlagsC rendition;
        CharacterColor foreground;
        CharacterColor background;
    };
    SavedState _savedState;

    // last position where we added a character
    int _lastPos;

    // used in REP (repeating char)
    quint32 _lastDrawnChar;
    std::unique_ptr<EscapeSequenceUrlExtractor> _escapeSequenceUrlExtractor;
    void toggleUrlInput();

    // Vt102Emulation defined max argument value that can be passed to a Screen function
    const int MAX_SCREEN_ARGUMENT = 40960;

    /* Graphics */
    void addPlacement(std::unique_ptr<TerminalGraphicsPlacement_t> &u);
    std::vector<std::unique_ptr<TerminalGraphicsPlacement_t>> _graphicsPlacements;
    void scrollPlacements(int n, qint64 below = INT64_MAX, qint64 above = INT64_MAX);
    bool _hasGraphics;

    //
    bool _ignoreWcWidth;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Screen::DecodingOptions)

}

#endif // SCREEN_H
