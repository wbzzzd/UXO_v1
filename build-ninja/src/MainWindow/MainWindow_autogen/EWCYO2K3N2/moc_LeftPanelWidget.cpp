/****************************************************************************
** Meta object code from reading C++ file 'LeftPanelWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.15)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../include/MainWindow/LeftPanelWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LeftPanelWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.15. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LeftPanelWidget_t {
    QByteArrayData data[17];
    char stringdata0[242];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_LeftPanelWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_LeftPanelWidget_t qt_meta_stringdata_LeftPanelWidget = {
    {
QT_MOC_LITERAL(0, 0, 15), // "LeftPanelWidget"
QT_MOC_LITERAL(1, 16, 14), // "targetSelected"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 16), // "Core::TargetInfo"
QT_MOC_LITERAL(4, 49, 6), // "target"
QT_MOC_LITERAL(5, 56, 19), // "targetDoubleClicked"
QT_MOC_LITERAL(6, 76, 26), // "refreshSimulationRequested"
QT_MOC_LITERAL(7, 103, 15), // "missionSelected"
QT_MOC_LITERAL(8, 119, 17), // "Core::MissionInfo"
QT_MOC_LITERAL(9, 137, 7), // "mission"
QT_MOC_LITERAL(10, 145, 14), // "deviceSelected"
QT_MOC_LITERAL(11, 160, 16), // "Core::DeviceInfo"
QT_MOC_LITERAL(12, 177, 6), // "device"
QT_MOC_LITERAL(13, 184, 16), // "onRefreshTargets"
QT_MOC_LITERAL(14, 201, 15), // "onFilterChanged"
QT_MOC_LITERAL(15, 217, 19), // "onSearchTextChanged"
QT_MOC_LITERAL(16, 237, 4) // "text"

    },
    "LeftPanelWidget\0targetSelected\0\0"
    "Core::TargetInfo\0target\0targetDoubleClicked\0"
    "refreshSimulationRequested\0missionSelected\0"
    "Core::MissionInfo\0mission\0deviceSelected\0"
    "Core::DeviceInfo\0device\0onRefreshTargets\0"
    "onFilterChanged\0onSearchTextChanged\0"
    "text"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LeftPanelWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       5,    1,   57,    2, 0x06 /* Public */,
       6,    0,   60,    2, 0x06 /* Public */,
       7,    1,   61,    2, 0x06 /* Public */,
      10,    1,   64,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    0,   67,    2, 0x0a /* Public */,
      14,    0,   68,    2, 0x0a /* Public */,
      15,    1,   69,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 11,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   16,

       0        // eod
};

void LeftPanelWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LeftPanelWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->targetSelected((*reinterpret_cast< const Core::TargetInfo(*)>(_a[1]))); break;
        case 1: _t->targetDoubleClicked((*reinterpret_cast< const Core::TargetInfo(*)>(_a[1]))); break;
        case 2: _t->refreshSimulationRequested(); break;
        case 3: _t->missionSelected((*reinterpret_cast< const Core::MissionInfo(*)>(_a[1]))); break;
        case 4: _t->deviceSelected((*reinterpret_cast< const Core::DeviceInfo(*)>(_a[1]))); break;
        case 5: _t->onRefreshTargets(); break;
        case 6: _t->onFilterChanged(); break;
        case 7: _t->onSearchTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LeftPanelWidget::*)(const Core::TargetInfo & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LeftPanelWidget::targetSelected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LeftPanelWidget::*)(const Core::TargetInfo & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LeftPanelWidget::targetDoubleClicked)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LeftPanelWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LeftPanelWidget::refreshSimulationRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (LeftPanelWidget::*)(const Core::MissionInfo & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LeftPanelWidget::missionSelected)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LeftPanelWidget::*)(const Core::DeviceInfo & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LeftPanelWidget::deviceSelected)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject LeftPanelWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_LeftPanelWidget.data,
    qt_meta_data_LeftPanelWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *LeftPanelWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LeftPanelWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LeftPanelWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int LeftPanelWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void LeftPanelWidget::targetSelected(const Core::TargetInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void LeftPanelWidget::targetDoubleClicked(const Core::TargetInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void LeftPanelWidget::refreshSimulationRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void LeftPanelWidget::missionSelected(const Core::MissionInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void LeftPanelWidget::deviceSelected(const Core::DeviceInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
