#include <QCoreApplication>
#include "equares_core/equares_script.h"
#include "equares_core/initBoxFactory.h"
#include "JsRunner.h"
#include "EquaresPrintUtil.h"

using namespace std;

static bool pickOption(QStringList& options, const QString& option) {
    if (options.contains(option)) {
        options.removeAll(option);
        return true;
    }
    else
        return false;
}

void describeSystem(QScriptEngine& engine, const QStringList& args, const QStringList& options_)
{
    QStringList options = options_;
    if (options.isEmpty())
        options << "ports" << "props" << "help";
    bool printPorts = pickOption(options, "ports");
    bool printProps = pickOption(options, "props");
    bool printHelp = pickOption(options, "help");
    if (!options.isEmpty())
        throw EquaresException(QString("Unknown describe system option(s): '%1'").arg(options.join(",")));
    Q_UNUSED(engine);
    QTextStream& os = EQUARES_COUT;
    if (args.empty()) {
        describeSystem(engine, BoxFactory::boxTypes() << "boxTypes", options);
        return;
    }
    QStringList boxTypes = BoxFactory::boxTypes();
    bool namedMode = args.size() > 1;
    foreach (const QString& arg, args) {
        if (arg == "boxTypes") {
            if (namedMode)
                os << "\nvar boxTypes = ";
            os << "[\n  ";
            printContainer(os, BoxFactory::boxTypes(), SimplePrinter<QString>(), ",\n  " );
            os << "\n]\n";
        }
        else {
            QScriptValue sbox;
            Box::Ptr boxptr;
            Box *box;
            if (boxTypes.contains(arg)) {
                boxptr = Box::Ptr(box = newBox(arg));
                sbox = engine.newQObject(box);
            }
            else {
                sbox = engine.evaluate(arg);
                if (sbox.isQObject())
                    box = qobject_cast<Box*>(sbox.toQObject());
                else
                    box = 0;
                if (!box)
                    throw EquaresException(QString("Failed to find box instance named '%1'").arg(arg));
            }
            if (namedMode)
                os << "\nvar " << arg << " = ";
            os << "{" << endl;
            bool needComma = false;
            if (printPorts) {
                os << "  inputs: [\n    ";
                printContainer(os, box->inputPorts(), PortPrinter(), ",\n    ");
                os << "\n  ],\n  outputs: [\n    ";
                printContainer(os, box->outputPorts(), PortPrinter(), ",\n    ");
                os << "\n  ]";
                needComma = true;
            }
            if (printProps) {
                if (needComma)
                    os << ",\n";
                os << "  properties: [\n    ";
                printContainer(os, box->boxProperties(), BoxPropPrinter(sbox), ",\n    ");
                os << "\n  ]";
                needComma = true;
            }
            if (printHelp) {
                if (!box->helpString().isEmpty()) {
                    if (needComma)
                        os << ",\n";
                    os << "  help: '" << escapeString(box->helpString()) << "'";
                    needComma = true;
                }
            }
            os << "\n}" << endl;
        }
    }
}

int main(int argc, char **argv)
{
#ifdef __linux__
    Q_INIT_RESOURCE(equares_core);
#endif // __linux__

    QCoreApplication app(argc, argv);

    try {
        // Parse command line arguments
        enum Mode { RunMode, DescribeMode, ServerMode } mode = RunMode;
        QStringList nonFlagArgs;
        bool forceInteractive = false;
        QStringList args = app.arguments();
        args.removeFirst();
        QStringList describeSystemOptions;
        foreach (QString arg, args) {
            if (arg.isEmpty())
                continue;
            if (arg[0] == '-' && arg.size() == 2) {
                switch (arg[1].toLatin1()) {
                case 'd':
                    mode = DescribeMode;
                    break;
                case 's':
                    mode = ServerMode;
                case 'i':
                    forceInteractive = true;
                    break;
                default:
                    throw EquaresException(QString("Unrecognized option '%1'").arg(arg));
                }
            }
            else if (arg.startsWith("-d")) {
                mode = DescribeMode;
                describeSystemOptions = arg.mid(2).split(",", QString::SkipEmptyParts);
            }
            else
                nonFlagArgs << arg;
        }

        // Create script engine
        QScriptEngine engine;

        // Create default thread manager
        DefaultThreadManager threadManager;

        // Initialize equares core
        initBoxFactory();
        registerEquaresScriptTypes(&engine);

        bool ok = true;

        switch (mode) {
        case RunMode:
            ok = JsRunner::runFiles(engine, nonFlagArgs, true);
            if (nonFlagArgs.isEmpty())
                // Standard input is already processed
                forceInteractive = false;
            break;
        case DescribeMode:
            break;
        case ServerMode:
            JsRunner::runServer(engine);
            forceInteractive = false;
            break;
        default:
            Q_ASSERT(false);
        }

        if (forceInteractive)
            // Process standard input (note: only runs if was ok)
            ok = ok && JsRunner::runFiles(engine, QStringList(), mode != DescribeMode);

        if (mode == DescribeMode && ok)
            describeSystem(engine, nonFlagArgs, describeSystemOptions);

        return ok? 0: -1;
    }
    catch(const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return -1;
    }
}

