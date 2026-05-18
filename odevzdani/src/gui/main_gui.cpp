/**
 * @file main_gui.cpp
 * @author Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)
 * @brief Vstupní bod GUI aplikace (main funkce s QApplication).
 */

#include <QApplication>
#include "main_window.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PetriNet Editor");
    app.setOrganizationName("ICP");

    MainWindow window;
    window.show();

    return app.exec();
}