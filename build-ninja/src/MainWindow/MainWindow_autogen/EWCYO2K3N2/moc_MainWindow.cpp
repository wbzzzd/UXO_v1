/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.15)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../include/MainWindow/MainWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.15. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[21];
    char stringdata0[408];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 16), // "on_actionNewTask"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 17), // "on_actionOpenPlan"
QT_MOC_LITERAL(4, 47, 17), // "on_actionSavePlan"
QT_MOC_LITERAL(5, 65, 13), // "on_actionExit"
QT_MOC_LITERAL(6, 79, 22), // "on_actionViewLeftPanel"
QT_MOC_LITERAL(7, 102, 23), // "on_actionViewRightPanel"
QT_MOC_LITERAL(8, 126, 22), // "on_actionViewStatusBar"
QT_MOC_LITERAL(9, 149, 23), // "on_actionSystemSettings"
QT_MOC_LITERAL(10, 173, 14), // "on_actionAbout"
QT_MOC_LITERAL(11, 188, 19), // "onNavigationChanged"
QT_MOC_LITERAL(12, 208, 5), // "index"
QT_MOC_LITERAL(13, 214, 16), // "onTargetSelected"
QT_MOC_LITERAL(14, 231, 16), // "Core::TargetInfo"
QT_MOC_LITERAL(15, 248, 6), // "target"
QT_MOC_LITERAL(16, 255, 21), // "onTargetDoubleClicked"
QT_MOC_LITERAL(17, 277, 28), // "onConfirmSimulationRequested"
QT_MOC_LITERAL(18, 306, 34), // "onStartSimulationDisposalRequ..."
QT_MOC_LITERAL(19, 341, 37), // "onCompleteSimulationDisposalR..."
QT_MOC_LITERAL(20, 379, 28) // "onRefreshSimulationRequested"

    },
    "MainWindow\0on_actionNewTask\0\0"
    "on_actionOpenPlan\0on_actionSavePlan\0"
    "on_actionExit\0on_actionViewLeftPanel\0"
    "on_actionViewRightPanel\0on_actionViewStatusBar\0"
    "on_actionSystemSettings\0on_actionAbout\0"
    "onNavigationChanged\0index\0onTargetSelected\0"
    "Core::TargetInfo\0target\0onTargetDoubleClicked\0"
    "onConfirmSimulationRequested\0"
    "onStartSimulationDisposalRequested\0"
    "onCompleteSimulationDisposalRequested\0"
    "onRefreshSimulationRequested"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x08 /* Private */,
       3,    0,   95,    2, 0x08 /* Private */,
       4,    0,   96,    2, 0x08 /* Private */,
       5,    0,   97,    2, 0x08 /* Private */,
       6,    0,   98,    2, 0x08 /* Private */,
       7,    0,   99,    2, 0x08 /* Private */,
       8,    0,  100,    2, 0x08 /* Private */,
       9,    0,  101,    2, 0x08 /* Private */,
      10,    0,  102,    2, 0x08 /* Private */,
      11,    1,  103,    2, 0x08 /* Private */,
      13,    1,  106,    2, 0x08 /* Private */,
      16,    1,  109,    2, 0x08 /* Private */,
      17,    0,  112,    2, 0x08 /* Private */,
      18,    0,  113,    2, 0x08 /* Private */,
      19,    0,  114,    2, 0x08 /* Private */,
      20,    0,  115,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_actionNewTask(); break;
        case 1: _t->on_actionOpenPlan(); break;
        case 2: _t->on_actionSavePlan(); break;
        case 3: _t->on_actionExit(); break;
        case 4: _t->on_actionViewLeftPanel(); break;
        case 5: _t->on_actionViewRightPanel(); break;
        case 6: _t->on_actionViewStatusBar(); break;
        case 7: _t->on_actionSystemSettings(); break;
        case 8: _t->on_actionAbout(); break;
        case 9: _t->onNavigationChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->onTargetSelected((*reinterpret_cast< const Core::TargetInfo(*)>(_a[1]))); break;
        case 11: _t->onTargetDoubleClicked((*reinterpret_cast< const Core::TargetInfo(*)>(_a[1]))); break;
        case 12: _t->onConfirmSimulationRequested(); break;
        case 13: _t->onStartSimulationDisposalRequested(); break;
        case 14: _t->onCompleteSimulationDisposalRequested(); break;
        case 15: _t->onRefreshSimulationRequested(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
