/****************************************************************************
** Meta object code from reading C++ file 'DetectionControlPanel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.15)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../include/MainWindow/DetectionControlPanel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DetectionControlPanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.15. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DetectionControlPanel_t {
    QByteArrayData data[5];
    char stringdata0[119];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DetectionControlPanel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DetectionControlPanel_t qt_meta_stringdata_DetectionControlPanel = {
    {
QT_MOC_LITERAL(0, 0, 21), // "DetectionControlPanel"
QT_MOC_LITERAL(1, 22, 26), // "confirmSimulationRequested"
QT_MOC_LITERAL(2, 49, 0), // ""
QT_MOC_LITERAL(3, 50, 32), // "startSimulationDisposalRequested"
QT_MOC_LITERAL(4, 83, 35) // "completeSimulationDisposalReq..."

    },
    "DetectionControlPanel\0confirmSimulationRequested\0"
    "\0startSimulationDisposalRequested\0"
    "completeSimulationDisposalRequested"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DetectionControlPanel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x06 /* Public */,
       3,    0,   30,    2, 0x06 /* Public */,
       4,    0,   31,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DetectionControlPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DetectionControlPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->confirmSimulationRequested(); break;
        case 1: _t->startSimulationDisposalRequested(); break;
        case 2: _t->completeSimulationDisposalRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DetectionControlPanel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DetectionControlPanel::confirmSimulationRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DetectionControlPanel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DetectionControlPanel::startSimulationDisposalRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DetectionControlPanel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DetectionControlPanel::completeSimulationDisposalRequested)) {
                *result = 2;
                return;
            }
        }
    }
    (void)_a;
}

QT_INIT_METAOBJECT const QMetaObject DetectionControlPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_DetectionControlPanel.data,
    qt_meta_data_DetectionControlPanel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DetectionControlPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DetectionControlPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DetectionControlPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int DetectionControlPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void DetectionControlPanel::confirmSimulationRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DetectionControlPanel::startSimulationDisposalRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void DetectionControlPanel::completeSimulationDisposalRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
