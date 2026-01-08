/**
 * @file Debugger.h
 * @brief Debugging macros and utilities.
 *
 * @project PacketForge
 * @author Ritesh Pandit (Riteshp2001)
 * @original_author Mohammad Ajmal Siddiqui (debugger macros)
 * @copyright Copyright (c) 2025 Ritesh Pandit. All rights reserved.
 * @license MIT License
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H
/**
 * @file debugger.h
 * @brief Header file for debugging macros.
 * @author Mohammad Ajmal Siddiqui
 *
 * The provided code is a debugger header file with macros for different types of debugging messages.
 * It allows you  to control the debugging behavior by modifying the predefined macros.
 *
 * Logging Macros:
 *   - DEBUG(x): Outputs the provided text x to the standard output stream (std::cout).
 *   - DEBUG_CODE(x): Wraps code blocks that should be conditionally included for debugging purposes.
 *
 * Usage Examples:
 *   ' DEBUG("Some Text") ' will output the text "Some Text" to the console if DEBUG_ENABLED is true.
 *   ' UNIT_A && DEBUG("SOME TEXT") ' is a logical AND operation where the right operand (DEBUG("SOME TEXT")) will only be evaluated and executed if UNIT_A is true and DEBUG_ENABLED is true.
 *   ' DEBUG_CODE({ // code... }) ' is used to wrap code blocks that should only be included when DEBUG_ENABLED is true. The code within the curly braces will be executed if DEBUG_ENABLED is true.
 *   ' #if UNIT_A and #endif ' are used to conditionally compile the code block within the DEBUG_CODE({ // code... }) when UNIT_A is true. This allows for additional conditional compilation based on specific modules or units.
 *
 * Note:
 *  The provided code is designed to be flexible and allows you to customize the logging behavior for different modules or units in your application.
 *  By enabling or disabling the macros, you can selectively include or exclude debugging statements to improve the performance of performance-sensitive applications.
*/


#define DEBUG_ENABLED           false
#define COMM_HANDLER_DEBUG      false
#define RX_TX_DEBUG             false
#define GLOBAL_VARIABLE_DEBUG   false

#if DEBUG_ENABLED
#include <QDebug>
#define DEBUG(x) [&](){qDebug()<< x;return true;}();
#define DEBUG_CODE(x) [&](){x return true;}();
#define LINE_INFO qDebug()<<__FILE__<<">"<<QString(__FUNCTION__).split("::").mid(0,2).join("::")<<">"<<__LINE__;

// --- Advanced Logging ---
#define LOG_INFO(msg) qDebug().noquote() << "[INFO]" << msg
#define LOG_WARN(msg) qDebug().noquote() << "[WARN]" << msg
#define LOG_ERROR(msg) qDebug().noquote() << "[ERROR]" << msg

// --- Assertions ---
#define ASSERT_X(cond, msg) if(!(cond)) { qDebug().noquote() << "[ASSERT FAIL]" << msg << "at" << __FILE__ << ":" << __LINE__; }

// --- Performance Timing ---
#include <QElapsedTimer>
class ScopedTimer {
    QString name;
    QElapsedTimer timer;
public:
    ScopedTimer(const QString& n) : name(n) { timer.start(); }
    ~ScopedTimer() { qDebug() << "[TIME]" << name << "took" << timer.nsecsElapsed()/1000000.0 << "ms"; }
};
#define MEASURE_TIME(name) ScopedTimer _timer(name)

#else
#define DEBUG(...) false;
#define DEBUG_CODE(...) false;
#define LINE_INFO
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define ASSERT_X(...)
#define MEASURE_TIME(...)
#endif

#endif // DEBUGGER_H
