#include "tarotcorrespondences.h"
#include <QMap>
#include <QStringList>

namespace TarotCorrespondences {

/*
int planetMajorNumber(const QString &planetId)
{

    static const QMap<QString, int> t = {
        {"Sun", 19}, {"Moon", 2}, {"Mercury", 1}, {"Venus", 3},
        {"Mars", 16}, {"Jupiter", 10}, {"Saturn", 21},
    };
    return t.value(planetId, -1);
}
*/

int planetMajorNumber(const QString &planetId)
{
    static const QMap<QString, int> t = {
        {"Sun", 19}, {"Moon", 2}, {"Mercury", 1}, {"Venus", 3},
        {"Mars", 16}, {"Jupiter", 10}, {"Saturn", 21},
        {"Uranus", 0}, {"Neptune", 12}, {"Pluto", 20},
    };
    return t.value(planetId, -1);
}

int signMajorNumber(const QString &sign)
{
    static const QMap<QString, int> t = {
        {"Aries", 4},        {"Taurus", 5},       {"Gemini", 6},
        {"Cancer", 7},       {"Leo", 8},          {"Virgo", 9},
        {"Libra", 11},       {"Scorpio", 13},     {"Sagittarius", 14},
        {"Capricorn", 15},   {"Aquarius", 17},    {"Pisces", 18},
    };
    return t.value(sign, -1);
}

int decanMinorNumber(const QString &sign, int decanIndex)
{
    if (decanIndex < 0 || decanIndex > 2) return -1;

    // Each element has three signs, each sign has three decans.
    // The decan's pip number within the suit runs 2..10 across the three signs:
    //   first sign of element:  decans 0,1,2 -> pips 2,3,4
    //   second sign:            decans 0,1,2 -> pips 5,6,7
    //   third sign:             decans 0,1,2 -> pips 8,9,10
    //
    // Suit base: Wands 22, Cups 36, Swords 50, Pentacles 64.
    // cardNumber = base + (pipNumber - 1)  because Ace is base+0.

    struct Info { int base; int firstPip; };
    /*
    static const QMap<QString, Info> t = {
        // Wands
        {"Aries",       {22, 2}}, {"Leo",         {22, 5}}, {"Sagittarius", {22, 8}},
        // Cups
        {"Cancer",      {36, 2}}, {"Scorpio",     {36, 5}}, {"Pisces",      {36, 8}},
        // Swords
        {"Gemini",      {50, 2}}, {"Libra",       {50, 5}}, {"Aquarius",    {50, 8}},
        // Pentacles
        {"Taurus",      {64, 2}}, {"Virgo",       {64, 5}}, {"Capricorn",   {64, 8}},
    };
    */

    static const QMap<QString, Info> t = {
        // Wands
        {"Aries",       {22, 2}}, {"Leo",         {22, 5}}, {"Sagittarius", {22, 8}},
        // Pentacles
        {"Taurus",      {64, 5}}, {"Virgo",       {64, 8}}, {"Capricorn",   {64, 2}},
        // Swords
        {"Gemini",      {50, 8}}, {"Libra",       {50, 2}}, {"Aquarius",    {50, 5}},
        // Cups
        {"Cancer",      {36, 2}}, {"Scorpio",     {36, 5}}, {"Pisces",      {36, 8}},
    };

    auto it = t.find(sign);
    if (it == t.end()) return -1;

    int pipNumber = it->firstPip + decanIndex;   // 2..10
    return it->base + (pipNumber - 1);
}

QString cardName(int number)
{
    static const QMap<int, QString> names = {
        {0,  "The Fool"},
        {1,  "The Magician"},
        {2,  "The Papess/High Priestess"},
        {3,  "The Empress"},
        {4,  "The Emperor"},
        {5,  "The Pope/Hierophant"},
        {6,  "The Lovers"},
        {7,  "The Chariot"},
        {8,  "Strength"},
        {9,  "The Hermit"},
        {10, "The Wheel"},
        {11, "Justice"},
        {12, "The Hanged Man"},
        {13, "Death"},
        {14, "Temperance"},
        {15, "The Devil"},
        {16, "The Tower"},
        {17, "The Star"},
        {18, "The Moon"},
        {19, "The Sun"},
        {20, "Judgement"},
        {21, "The World"},

        {22, "ace of wands"},    {23, "two of wands"},    {24, "three of wands"},
        {25, "four of wands"},   {26, "five of wands"},   {27, "six of wands"},
        {28, "seven of wands"},  {29, "eight of wands"},  {30, "nine of wands"},
        {31, "ten of wands"},    {32, "page of wands"},   {33, "knight of wands"},
        {34, "queen of wands"},  {35, "king of wands"},

        {36, "ace of cups"},     {37, "two of cups"},     {38, "three of cups"},
        {39, "four of cups"},    {40, "five of cups"},    {41, "six of cups"},
        {42, "seven of cups"},   {43, "eight of cups"},   {44, "nine of cups"},
        {45, "ten of cups"},     {46, "page of cups"},    {47, "knight of cups"},
        {48, "queen of cups"},   {49, "king of cups"},

        {50, "ace of swords"},   {51, "two of swords"},   {52, "three of swords"},
        {53, "four of swords"},  {54, "five of swords"},  {55, "six of swords"},
        {56, "seven of swords"}, {57, "eight of swords"}, {58, "nine of swords"},
        {59, "ten of swords"},   {60, "page of swords"},  {61, "knight of swords"},
        {62, "queen of swords"}, {63, "king of swords"},

        {64, "ace of coins"},    {65, "two of coins"},    {66, "three of coins"},
        {67, "four of coins"},   {68, "five of coins"},   {69, "six of coins"},
        {70, "seven of coins"},  {71, "eight of coins"},  {72, "nine of coins"},
        {73, "ten of coins"},    {74, "page of coins"},   {75, "knight of coins"},
        {76, "queen of coins"},  {77, "king of coins"},
    };
    return names.value(number, QString("Card %1").arg(number));
}

int courtCardNumber(const QString &sign, int decanIndex)
{
    if (decanIndex < 0 || decanIndex > 2) return -1;

    static const QStringList signs = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
        "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces"
    };
    int signIdx = signs.indexOf(sign);
    if (signIdx < 0) return -1;

    // courts in chain order, indexed by the sign whose decan 1 and 2 they cover
    static const int courtNumbers[12] = {
        34,  // Aries   I, II   → Queen of Wands
        75,  // Taurus  I, II   → Knight of Coins
        63,  // Gemini  I, II   → King of Swords
        48,  // Cancer  I, II   → Queen of Cups
        33,  // Leo     I, II   → Knight of Wands
        77,  // Virgo   I, II   → King of Coins
        62,  // Libra   I, II   → Queen of Swords
        47,  // Scorpio I, II   → Knight of Cups
        35,  // Sagittarius I, II → King of Wands
        76,  // Capricorn I, II → Queen of Coins
        61,  // Aquarius I, II  → Knight of Swords
        49,  // Pisces  I, II   → King of Cups
    };

    if (decanIndex < 2)
        return courtNumbers[signIdx];
    else
        return courtNumbers[(signIdx + 1) % 12];
}

int aceForSign(const QString &sign)
{
    static const QMap<QString, int> t = {
        {"Aries", 64}, {"Taurus", 64}, {"Gemini", 64},
        {"Cancer", 22}, {"Leo", 22}, {"Virgo", 22},
        {"Libra", 36}, {"Scorpio", 36}, {"Sagittarius", 36},
        {"Capricorn", 50}, {"Aquarius", 50}, {"Pisces", 50},
    };
    return t.value(sign, -1);
}

int pageForSign(const QString &sign)
{
    static const QMap<QString, int> t = {
        {"Aries", 74}, {"Taurus", 74}, {"Gemini", 74},
        {"Cancer", 32}, {"Leo", 32}, {"Virgo", 32},
        {"Libra", 46}, {"Scorpio", 46}, {"Sagittarius", 46},
        {"Capricorn", 60}, {"Aquarius", 60}, {"Pisces", 60},
    };
    return t.value(sign, -1);
}

} // namespace TarotCorrespondences
