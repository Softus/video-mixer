#ifndef TRANSCYRILLIC_H
#define TRANSCYRILLIC_H

#include <QString>

// Convert russian names written in latin symbols back to cyrillic
//
// IVANOV => ИВАНОВ
// NEPOMNYASHCHIKH => НЕПОМНЯЩИХ
//
QString translateToCyrillic(const QString& str);

#endif // TRANSCYRILLIC_H
