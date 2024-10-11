#include "ConsoleTable.h"


ConsoleTable::ConsoleTable(const std::initializer_list<std::string> headers) : headers{headers} {
    for (const auto &column : headers) {
        widths.push_back(column.length());
    }
}

void ConsoleTable::setPadding(unsigned int n) {
    padding = n;
}

void ConsoleTable::setTittle(std::string n) {
    tittle = n;
}

void ConsoleTable::setStyle(unsigned int n) {
    switch (n) {
    case 0 :
        style = BasicStyle;
        break;
    case 1 :
        style = LineStyle;
        break;
    case 2 :
        style = DoubleLineStyle;
        break;
    case 3 :
        style = InvisibleStyle;
        break;
    case 4 :
        style = dufStyle;
        break;
    default :
        style = BasicStyle;
        break;
    }
}

bool ConsoleTable::addRow(std::initializer_list<std::string> row) {
    if (row.size() > widths.size()) {
        throw std::invalid_argument{"Appended row size must be same as header size"};
    }

    auto r = std::vector<std::string>{row};
    rows.push_back(r);
    for (unsigned int i = 0; i < r.size(); ++i) {
        widths[i] = std::max(r[i].size() - searchColor(r[i]), widths[i]);
    }
    return true;
}


bool ConsoleTable::removeRow(unsigned int index) {
    if (index > rows.size())
        return false;

    rows.erase(rows.begin() + index);
    return true;
}


ConsoleTable &ConsoleTable::operator+=(std::initializer_list<std::string> row) {
    if (row.size() > widths.size()) {
        throw std::invalid_argument{"Appended row size must be same as header size"};
    }

    addRow(row);
    return *this;
}


ConsoleTable &ConsoleTable::operator-=(const uint32_t rowIndex) {
    if (rows.size() < rowIndex)
        throw std::out_of_range{"Row index out of range."};

    removeRow(rowIndex);
    return *this;
}

std::string ConsoleTable::getLine(RowType rowType) const {
    std::stringstream line;
    line << rowType.left;
    for (unsigned int i = 0; i < widths.size(); ++i) {
        for (unsigned int j = 0; j < (widths[i] + padding + padding); ++j) {
            line << style.horizontal;
        }
        line << (i == widths.size() - 1 ? rowType.right : rowType.intersect);
    }
    return line.str() + "\n";
}


std::string ConsoleTable::getHeaders(const Headers &headers) const {
    std::stringstream line;
    line << style.vertical;
    for (unsigned int i = 0; i < headers.size(); ++i) {
        std::string text = headers[i];
        line << SPACE_CHARACTER * padding + text + SPACE_CHARACTER * (widths[i] - text.length() + searchColor(text)) + SPACE_CHARACTER * padding;
        line << style.vertical;
    }
    line << "\n";
    return line.str();
}

size_t ConsoleTable::calculateSizeRow(void) const{
    size_t total = 1;
    for (unsigned int i = 0; i < headers.size(); ++i)
        total = total + 5 + widths[i];
    return total;
}

size_t ConsoleTable::simulateSizeRow(std::string text, int diference) const{
    size_t total = 1;
    total = total + 5 + text.length() + (widths[0] - calculateDiference(diference, widths[0], text));
    for (unsigned int i = 1; i < headers.size(); ++i)
        total = total + 5 + (widths[i] - calculateDiference(diference, widths[i], text));
    return total;
}

size_t ConsoleTable::calculateDiference(int &diference, size_t width, const std::string &text) const{
    size_t operatorDiference = 0;
    if (diference > 0){
        diference = 0;
        operatorDiference = text.length();
    } else if (diference < 0){
        if (abs(diference) < width){
            operatorDiference = width - abs(diference);
            diference = 0;
        } else{
            diference = diference + width;
            operatorDiference = width;
        }
    }
    return operatorDiference;
}

std::string ConsoleTable::getTittle(const std::string &tittle) const {
    std::stringstream line;
    line << style.vertical;
    std::string text = tittle;
    int diference = 0;
    diference = widths[0] - text.length() + searchColor(text);
    size_t normalRow = calculateSizeRow();
    size_t actualRow = simulateSizeRow(text, diference);
    int sizeRowDiference = abs(normalRow - actualRow);
    for (unsigned int i = 0; i < headers.size(); ++i){
        size_t operatorDiference = calculateDiference(diference, widths[i], text);
        if(i > 0)
            line << SPACE_CHARACTER;
        line << SPACE_CHARACTER * padding;
        if(i > 0){
            if (i == headers.size() - 1)
                line << SPACE_CHARACTER * (widths[i] - operatorDiference - sizeRowDiference);
            else
                line << SPACE_CHARACTER * (widths[i] - operatorDiference);
        } else {
            line << text +
                SPACE_CHARACTER * (widths[i] - operatorDiference + searchColor(text));
        }
        line << SPACE_CHARACTER * padding;
    }
    line << style.vertical;
    line << "\n";
    return line.str();
}

std::string ConsoleTable::getRows(const Rows &rows) const {
    std::stringstream line;
    for (const auto &row : rows) {
        line << style.vertical;
        for (unsigned int j = 0; j < row.size(); ++j) {
            std::string text = row[j];
            line << SPACE_CHARACTER * padding + text + SPACE_CHARACTER * (widths[j] - text.length() + searchColor(text)) + SPACE_CHARACTER * padding;
            line << style.vertical;
        }
        line << "\n";
    }

    return line.str();
}

std::ostream &operator<<(std::ostream &out, const ConsoleTable &consoleTable) {
    out << consoleTable.getLine(consoleTable.style.topTittle);
    out << consoleTable.getTittle(consoleTable.tittle);
    out << consoleTable.getLine(consoleTable.style.middleTittle);
    out << consoleTable.getHeaders(consoleTable.headers);
    out << consoleTable.getLine(consoleTable.style.middle);
    out << consoleTable.getRows(consoleTable.rows);
    out << consoleTable.getLine(consoleTable.style.bottom);
    return out;
}

bool ConsoleTable::sort(bool ascending) {
    if (ascending)
        std::sort(rows.begin(), rows.end(), std::less<std::vector<std::string>>());
    else
        std::sort(rows.begin(), rows.end(), std::greater<std::vector<std::string>>());
    return true;
}

void ConsoleTable::updateRow(unsigned int row, unsigned int header, const std::string &data) {
    if (row > rows.size() - 1)
        throw std::out_of_range{"Row index out of range."};
    if (header > headers.size() - 1)
        throw std::out_of_range{"Header index out of range."};

    rows[row][header] = data;
}

void ConsoleTable::updateHeader(unsigned int header, const std::string &text) {
    if (header > headers.size())
        throw std::out_of_range{"Header index out of range."};

    headers[header] = text;
}

size_t ConsoleTable::searchColor(const std::string &text) const{
    size_t pos = text.find(COLOR_INITIATOR_CHARACTER,0);
    size_t counter = 0;
    if (pos != std::string::npos){
        counter = countCharacters(counter, pos, text, COLOR_FINAL_CHARACTER);
        size_t rpos = text.rfind("\e");
        if (rpos  != std::string::npos && rpos > pos)
            counter = countCharacters(counter, rpos, text, COLOR_FINAL_CHARACTER);
        return counter;
    }
    return counter;
}

std::string operator*(const std::string &other, int repeats) {
    std::string ret;
    ret.reserve(other.size() * repeats);
    for (; repeats; --repeats)
        ret.append(other);
    return ret;
}

size_t countCharacters(size_t counter, size_t pos,const std::string &text, char characterBreaker){
    counter++;
    for(size_t i = pos; i < text.length(); i++){
        if(text[i] == characterBreaker)
            break;
        counter++;
    }
    return counter;
}
