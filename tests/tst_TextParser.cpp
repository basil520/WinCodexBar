#include "tst_TextParser.h"
#include <QtTest>
#include "util/TextParser.h"

void TextParserTest::testStripAnsiEscapes() {
    QString input = "\x1b[1mHello\x1b[0m World\x1b[31m!\x1b[0m";
    QString result = TextParser::stripAnsiEscapes(input);
    QCOMPARE(result, QString("Hello World!"));
}

void TextParserTest::testStripAnsiEscapesNoAnsi() {
    QString input = "Hello World";
    QString result = TextParser::stripAnsiEscapes(input);
    QCOMPARE(result, QString("Hello World"));
}

void TextParserTest::testExtractPercent() {
    double result = TextParser::extractPercent("Usage: 75% left");
    QCOMPARE(result, 75.0);
}

void TextParserTest::testExtractPercentDecimal() {
    double result = TextParser::extractPercent("Usage: 33.5% remaining");
    QCOMPARE(result, 33.5);
}

void TextParserTest::testExtractPercentNotFound() {
    double result = TextParser::extractPercent("No usage data");
    QCOMPARE(result, 0.0);
}

void TextParserTest::testExtractResetDescription() {
    QString result = TextParser::extractResetDescription("resets in 2 hours");
    QCOMPARE(result, QString("in 2 hours"));
}

void TextParserTest::testExtractResetDescriptionNotFound() {
    QString result = TextParser::extractResetDescription("No quota info available");
    QVERIFY(result.isEmpty());
}

void TextParserTest::testStripCRLF() {
    QString input = "line1\r\nline2\r\n";
    QString result = TextParser::stripAnsiEscapes(input);
    QCOMPARE(result, QString("line1\nline2\n"));
}

QTEST_MAIN(TextParserTest)
