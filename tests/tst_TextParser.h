#pragma once

#include <QObject>

class TextParserTest : public QObject {
    Q_OBJECT

private slots:
    void testStripAnsiEscapes();
    void testStripTerminalControlSequencesFromCodexTui();
    void testStripAnsiEscapesNoAnsi();
    void testExtractPercent();
    void testExtractPercentDecimal();
    void testExtractPercentNotFound();
    void testExtractResetDescription();
    void testExtractResetDescriptionNotFound();
    void testStripCRLF();
};
