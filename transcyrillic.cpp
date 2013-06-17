#include "transcyrillic.h"

static inline int next(const QString& str, int idx)
{
    return str.length() > ++idx? str[idx].toLower().unicode(): 0;
}

QString translateToCyrillic(const QString& str)
{
    if (str.isNull())
    {
        return str;
    }

    QString ret;

    for (int i = 0; i < str.length(); ++i)
    {
        switch (str[i].unicode())
        {
        case 'A':
            ret.append(L'А');
            break;
        case 'B':
            ret.append(L'Б');
            break;
        case 'V':
            ret.append(L'В');
            break;
        case 'G':
            ret.append(L'Г');
            break;
        case 'D':
            ret.append(L'Д');
            break;
        case 'E':
            ret.append(i==0?L'Э':L'Е'); // В начале слова русская 'е' это 'ye'
            break;
        case 'Z':
            ret.append(next(str, i) == 'h'? ++i, L'Ж': L'З');
            break;
        case 'I':
            ret.append(L'И');
            break;
        case 'K':
            ret.append(next(str, i) == 'h'? ++i, L'Х': L'К');
            break;
        case 'L':
            ret.append(L'Л');
            break;
        case 'M':
            ret.append(L'М');
            break;
        case 'N':
            ret.append(L'Н');
            break;
        case 'O':
            ret.append(L'О');
            break;
        case 'P':
            ret.append(L'П');
            break;
        case 'R':
            ret.append(L'Р');
            break;
        case 'S':
            if (next(str, i) == 'h')
            {
                ++i;
                if (next(str, i) == 'c' && next(str, i + 1) == 'h')
                {
                    i+=2;
                    ret.append(L'Щ');
                }
                else
                {
                    ret.append(L'Ш');
                }
            }
            else
            {
                ret.append(L'С');
            }
            break;
        case 'T':
            ret.append(next(str, i) == 's'? ++i, L'Ц': L'Т');
            break;
        case 'U':
            ret.append(L'У');
            break;
        case 'F':
            ret.append(L'Ф');
            break;
        case 'X':
            ret.append(L'К').append(i < str.length() - 1 && str[i+1].isLower()? L'с': L'С');
            break;
        case 'C':
            ret.append(next(str, i) == 'h'? ++i, L'Ч': L'К'); // Английская 'c' без 'h' не должна встрачаться.
            break;
        case 'Y':
            switch (next(str, i)) {
            case 'e':
                ++i, ret.append(L'Е');
                break;
            case 'o':
                ++i, ret.append(L'Ё');
                break;
            case 'u':
                ++i, ret.append(L'Ю');
                break;
            case 'i':
                ret.append(L'Ь'); // Только для мягкого знака перед 'и', как Ilyin.
                break;
            case 'a':
                ++i, ret.append(L'Я');
                break;
            case '\x0':
            case ' ':
            case '.':
                ret.append(L'И').append(L'Й'); // Фамилии на ый заканчиваются редко
                break;
            default:
                ret.append(L'Ы');
                break;
            }
            break;

        case 'a':
            ret.append(L'а');
            break;
        case 'b':
            ret.append(L'б');
            break;
        case 'v':
            ret.append(L'в');
            break;
        case 'g':
            ret.append(L'г');
            break;
        case 'd':
            ret.append(L'д');
            break;
        case 'e':
            ret.append(i==0?L'э':L'е'); // В начале слова русская 'е' это 'ye'
            break;
        case 'z':
            ret.append(next(str, i) == 'h'? ++i, L'ж': L'з');
            break;
        case 'i':
            ret.append(L'и');
            break;
        case 'k':
            ret.append(next(str, i) == 'h'? ++i, L'х': L'к');
            break;
        case 'l':
            ret.append(L'л');
            break;
        case 'm':
            ret.append(L'м');
            break;
        case 'n':
            ret.append(L'н');
            break;
        case 'o':
            ret.append(L'о');
            break;
        case 'p':
            ret.append(L'п');
            break;
        case 'r':
            ret.append(L'р');
            break;
        case 's':
            if (next(str, i) == 'h')
            {
                ++i;
                if (next(str, i) == 'c' && next(str, i + 1) == 'h')
                {
                    i+=2;
                    ret.append(L'щ');
                }
                else
                {
                    ret.append(L'ш');
                }
            }
            else
            {
                ret.append(L'с');
            }
            break;
        case 't':
            ret.append(next(str, i) == 's'? ++i, L'ц': L'т');
            break;
        case 'u':
            ret.append(L'у');
            break;
        case 'f':
            ret.append(L'ф');
            break;
        case 'x':
            ret.append(L'к').append(L'с');
            break;
        case 'с':
            ret.append(next(str, i) == 'h'? ++i, L'ч': L'к'); // Английская 'c' без 'h' не должна встрачаться.
            break;
        case 'y':
            switch (next(str, i)) {
            case 'e':
                ++i, ret.append(L'е');
                break;
            case 'o':
                ++i, ret.append(L'ё');
                break;
            case 'u':
                ++i, ret.append(L'ю');
                break;
            case 'i':
                ret.append(L'ь'); // Только для мягкого знака перед 'и', как Ilyin.
                break;
            case 'a':
                ++i, ret.append(L'я');
                break;
            case '\x0':
            case ' ':
            case '.':
                ret.append(L'и').append(L'й'); // Фамилии на ый заканчиваются редко
                break;
            default:
                ret.append(L'ы');
                break;
            }
            break;

        default:
            ret.append(str[i]);
            break;
        }
    }

    return ret;
}
