#ifndef CONSOLETABLE_CONSOLETABLE_H
#define CONSOLETABLE_CONSOLETABLE_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cmath>

class ConsoleTable {
public:

    typedef std::vector<std::string> Headers;
    typedef std::vector<std::vector<std::string>> Rows;
    typedef std::vector<std::size_t> Widths;

    /// Initialize a new ConsoleTable
    /// \param headers Stringlist of the tables headers
    ConsoleTable(const std::initializer_list<std::string> headers);


    /// Sets the distance from the text to the cell border
    /// \param n Spaces between the text and the cell border
    void setPadding(unsigned int n);

    void setTittle(std::string n);


    /// Sets the style of the table, default is 0
    /// n = 0 : Basic table style
    /// n = 1 : Single lined table style
    /// n = 2 : Double lined table style
    /// n = 3 : Invisivle table lines style
    /// \param n The table style number
    void setStyle(unsigned int n);


    /// Sorts the table rows based on the first column
    /// \param ascending Should table be sorted ascending or descending
    /// \return True if sorting was successful, otherwise false
    bool sort(bool ascending);


    /// Adds a new row to the table
    /// \param row A list of strings to add as row
    /// \return True if the value was added successfully, otherwise false
    bool addRow(std::initializer_list<std::string> row);


    /// Removes a row from the table by the row index
    /// \param index The index of the row that should be removed
    /// \return True if the row was removed successfully, otherwise false
    bool removeRow(unsigned int index);


    /// Update an existing table cell with new data
    /// \param row The index of the row that needs to be updated
    /// \param header The index of the column that needs to be updated
    /// \param data The new data that should be assigned to teh cell
    void updateRow(unsigned int row, unsigned int header,const std::string &data);


    /// Update a header with new text
    /// \param header Index of the header that should be updated
    /// \param text The new teext of the new header
    void updateHeader(unsigned int header,const std::string &text);


    /// Operator of the addRow() function
    /// \param row A list of strings to add as row
    /// \return this
    ConsoleTable &operator+=(std::initializer_list<std::string> row);


    /// Operator of the removeRow() function
    /// \param rowIndex The index of the row that should be removed
    /// \return this
    ConsoleTable &operator-=(uint32_t rowIndex);


private:

    /// Holds all header strings of the table
    Headers headers;

    std::string tittle = "";

    /// Holds all rows of the table
    Rows rows;


    /// Holds the size of widest string of each column of the table
    Widths widths;

    /// Defines row type
    struct RowType {
        std::string left;
        std::string intersect;
        std::string right;
    };

    /// Defines table style rows (top, middle, bottom etc)
    struct TableStyle {
        std::string horizontal;
        std::string vertical;
        RowType topTittle;
        RowType middleTittle;
        RowType top;
        RowType middle;
        RowType bottom;
    };


    /// Basic style - works on all systems, used as default style
    TableStyle BasicStyle = {"-", "|", {"+", "-", "+"}, {"+", "+", "+"}, {"+", "+", "+"}, {"+", "+", "+"}, {"+", "+", "+"}};


    /// Single lined style - requires speecial character support
    TableStyle LineStyle = {"━", "┃", {"┏", "━", "┓"}, {"┣", "┳", "┫"}, {"┏", "┳", "┓"}, {"┣", "╋", "┫"}, {"┗", "┻", "┛"}};

    /// Single duf lined style - requires speecial character support
    /// Nerd Font
    TableStyle dufStyle = {"─", "│", {"╭", "─", "╮"}, {"├", "┬", "┤"}, {"╭", "┬", "╮"}, {"├", "┼", "┤"}, {"╰", "┴", "╯"}};


    /// Single double style - requires speecial character support
    TableStyle DoubleLineStyle = {"═", "║", {"╔", "═", "╗"}, {"╠", "╦", "╣"},{"╔", "╦", "╗"}, {"╠", "╬", "╣"}, {"╚", "╩", "╝"}};


    /// No visible table outlines - works on all systems
    TableStyle InvisibleStyle = {" ", " ", {" ", " ", " "}, {" ", " ", " "}, {" ", " ", " "}, {" ", " ", " "}, {" ", " ", " "}};


    /// Current table style
    TableStyle style = BasicStyle;


    /// Space character constant
    const std::string SPACE_CHARACTER = " ";

    /// Color initiator character constant
    const std::string COLOR_INITIATOR_CHARACTER = "\e";

    /// Color final character constant
    const char COLOR_FINAL_CHARACTER = 'm';

    /// The distance between the cell text and the cell border
    unsigned int padding = 1;


    /// Returns a formatted horizontal separation line for the table
    /// \param rowType The type of the row (top, middle, bottom)
    /// \return The formatted row string
    std::string getLine(RowType rowType) const;

    size_t calculateDiference(int &diference, size_t width, const std::string &text) const;

    size_t calculateSizeRow(void) const;

    size_t simulateSizeRow(std::string text, int diference) const;

    /// Returns a formatted header string
    /// \param headers The Headers-object that holds the header strings
    /// \return The formatted header string
    std::string getHeaders(const Headers &headers) const;


    /// Returns a formmatted row string
    /// \param rows The Rows-object that holds all rows of the table
    /// \return A formatted string of all rows in the table
    std::string getRows(const Rows &rows) const;

    /// Returns a formmatted title row string
    /// \param tittle The ttitle row of the table
    /// \return A formatted string of a tittle rows in the table
    std::string getTittle(const std::string &tittle) const;


    /// Writes the entire table with all its contents in the output stream
    /// This can be used to display the table using the std::cout function
    /// \param out The output stream the table should be written to
    /// \param consoleTable The ConsoleTable-object
    /// \return Output stream with the formatted table string
    friend std::ostream &operator<<(std::ostream &out, const ConsoleTable &consoleTable);

    /// Search color into the introduced text, return the number of
    /// characters that you need to remove from the size.
    /// \param text variable that contain text to analize
    /// \return number of characters to remove from the size
    size_t searchColor(const std::string &text) const;
};


/// Repeats a given string n times
/// \param other The string to repeat
/// \param repeats The amount the string should be repeated
/// \return The repeated string
std::string operator*(const std::string &other, int repeats);

/// Count characters from position to characterBreaker and previous count
/// \param counter this variable is the previous count
/// \param pos is the position of the string that you need to start
/// \param text strint to analize
/// \param characterBreaker character to break the count
/// \return return the count of characters
size_t countCharacters(size_t counter, size_t pos,const std::string &text, char characterBreaker) ;

#endif //CONSOLETABLE_CONSOLETABLE_H
