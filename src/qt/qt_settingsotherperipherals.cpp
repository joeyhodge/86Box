/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box distribution.
 *
 *          Other peripherals configuration UI module.
 *
 *
 *
 * Authors: Joakim L. Gilje <jgilje@jgilje.net>
 *
 *          Copyright 2021 Joakim L. Gilje
 */
#include "qt_settingsotherperipherals.hpp"
#include "ui_qt_settingsotherperipherals.h"

extern "C" {
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/machine.h>
#include <86box/isamem.h>
#include <86box/isartc.h>
#include <86box/unittester.h>
#include <86box/novell_cardkey.h>
}

#include "qt_deviceconfig.hpp"
#include "qt_models_common.hpp"

SettingsOtherPeripherals::SettingsOtherPeripherals(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsOtherPeripherals)
{
    ui->setupUi(this);
    onCurrentMachineChanged(machine);
}

void
SettingsOtherPeripherals::onCurrentMachineChanged(int machineId)
{
    this->machineId = machineId;

    bool machineHasIsa = (machine_has_bus(machineId, MACHINE_BUS_ISA) > 0);
    ui->checkBoxISABugger->setChecked((machineHasIsa && (bugger_enabled > 0)) ? true : false);
    ui->checkBoxPOSTCard->setChecked(postcard_enabled > 0 ? true : false);
    ui->checkBoxUnitTester->setChecked(unittester_enabled > 0 ? true : false);
    ui->checkBoxKeyCard->setChecked((machineHasIsa && (novell_keycard_enabled > 0)) ? true : false);
    ui->checkBoxISABugger->setEnabled(machineHasIsa);
    ui->checkBoxKeyCard->setEnabled(machineHasIsa);
    ui->pushButtonConfigureKeyCard->setEnabled(novell_keycard_enabled > 0);
    ui->pushButtonConfigureUT->setEnabled(unittester_enabled > 0);
    ui->comboBoxRTC->setEnabled(machineHasIsa);
    ui->pushButtonConfigureRTC->setEnabled(machineHasIsa);

    ui->comboBoxCard1->clear();
    ui->comboBoxCard2->clear();
    ui->comboBoxCard3->clear();
    ui->comboBoxCard4->clear();
    ui->comboBoxRTC->clear();

    auto *model       = ui->comboBoxRTC->model();
    int   d           = 0;
    int   selectedRow = 0;
    while (true) {
        QString name = DeviceConfig::DeviceName(isartc_get_device(d), isartc_get_internal_name(d), 0);
        if (name.isEmpty()) {
            break;
        }

        if (!device_is_valid(isartc_get_device(d), machineId)) {
            break;
        }

        int row = Models::AddEntry(model, name, d);
        if (d == isartc_type) {
            selectedRow = row;
        }
        ++d;
    }
    ui->comboBoxRTC->setCurrentIndex(selectedRow);
    ui->pushButtonConfigureRTC->setEnabled((isartc_type != 0) && isartc_has_config(isartc_type) && machineHasIsa);

    // ISA Memory Expansion Card
    QComboBox *         cbox[ISAMEM_MAX]         = { 0 };
    QAbstractItemModel *models[ISAMEM_MAX]       = { 0 };
    int                 removeRows_[ISAMEM_MAX]  = { 0 };
    int                 selectedRows[ISAMEM_MAX] = { 0 };

    for (uint8_t c = 0; c < ISAMEM_MAX; ++c) {
        cbox[c]        = findChild<QComboBox *>(QString("comboBoxCard%1").arg(c + 1));
        models[c]      = cbox[c]->model();
        removeRows_[c] = models[c]->rowCount();
    }

    d = 0;
    while (true) {
        const QString name = DeviceConfig::DeviceName(isamem_get_device(d),
                                                      isamem_get_internal_name(d), 0);

        if (name.isEmpty())
            break;

        if (device_is_valid(isamem_get_device(d), machineId)) {
            for (uint8_t c = 0; c < ISAMEM_MAX; ++c) {
                int row = Models::AddEntry(models[c], name, d);

                if (d == isamem_type[c])
                    selectedRows[c] = row - removeRows_[c];
            }
        }

       d++;
    }

    for (uint8_t c = 0; c < ISAMEM_MAX; ++c) {
        models[c]->removeRows(0, removeRows_[c]);
        cbox[c]->setEnabled(models[c]->rowCount() > 1);
        cbox[c]->setCurrentIndex(-1);
        cbox[c]->setCurrentIndex(selectedRows[c]);
        findChild<QPushButton *>(QString("pushButtonConfigureCard%1").arg(c + 1))->setEnabled((isamem_type[c] != 0) &&
                                isamem_has_config(isamem_type[c]) && machineHasIsa);
    }
}

SettingsOtherPeripherals::~SettingsOtherPeripherals()
{
    delete ui;
}

void
SettingsOtherPeripherals::save()
{
    /* Other peripherals category */
    bugger_enabled         = ui->checkBoxISABugger->isChecked() ? 1 : 0;
    postcard_enabled       = ui->checkBoxPOSTCard->isChecked() ? 1 : 0;
    unittester_enabled     = ui->checkBoxUnitTester->isChecked() ? 1 : 0;
    novell_keycard_enabled = ui->checkBoxKeyCard->isChecked() ? 1 : 0;
    isartc_type            = ui->comboBoxRTC->currentData().toInt();

    /* ISA memory boards. */
    for (int i = 0; i < ISAMEM_MAX; i++) {
        auto *cbox     = findChild<QComboBox *>(QString("comboBoxCard%1").arg(i + 1));
        isamem_type[i] = cbox->currentData().toInt();
    }
}

void
SettingsOtherPeripherals::on_comboBoxRTC_currentIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    ui->pushButtonConfigureRTC->setEnabled((index != 0) && isartc_has_config(index) && machine_has_bus(machineId, MACHINE_BUS_ISA));
}

void
SettingsOtherPeripherals::on_pushButtonConfigureRTC_clicked()
{
    DeviceConfig::ConfigureDevice(isartc_get_device(ui->comboBoxRTC->currentData().toInt()));
}

void
SettingsOtherPeripherals::on_comboBoxCard1_currentIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    ui->pushButtonConfigureCard1->setEnabled((index != 0) && isamem_has_config(index) && machine_has_bus(machineId, MACHINE_BUS_ISA));
}

void
SettingsOtherPeripherals::on_pushButtonConfigureCard1_clicked()
{
    DeviceConfig::ConfigureDevice(isamem_get_device(ui->comboBoxCard1->currentData().toInt()), 1);
}

void
SettingsOtherPeripherals::on_comboBoxCard2_currentIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    ui->pushButtonConfigureCard2->setEnabled((index != 0) && isamem_has_config(index) && machine_has_bus(machineId, MACHINE_BUS_ISA));
}

void
SettingsOtherPeripherals::on_pushButtonConfigureCard2_clicked()
{
    DeviceConfig::ConfigureDevice(isamem_get_device(ui->comboBoxCard2->currentData().toInt()), 2);
}

void
SettingsOtherPeripherals::on_comboBoxCard3_currentIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    ui->pushButtonConfigureCard3->setEnabled((index != 0) && isamem_has_config(index) && machine_has_bus(machineId, MACHINE_BUS_ISA));
}

void
SettingsOtherPeripherals::on_pushButtonConfigureCard3_clicked()
{
    DeviceConfig::ConfigureDevice(isamem_get_device(ui->comboBoxCard3->currentData().toInt()), 3);
}

void
SettingsOtherPeripherals::on_comboBoxCard4_currentIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    ui->pushButtonConfigureCard4->setEnabled((index != 0) && isamem_has_config(index) && machine_has_bus(machineId, MACHINE_BUS_ISA));
}

void
SettingsOtherPeripherals::on_pushButtonConfigureCard4_clicked()
{
    DeviceConfig::ConfigureDevice(isamem_get_device(ui->comboBoxCard4->currentData().toInt()), 4);
}

void
SettingsOtherPeripherals::on_checkBoxUnitTester_stateChanged(int arg1)
{
    ui->pushButtonConfigureUT->setEnabled(arg1 != 0);
}

void
SettingsOtherPeripherals::on_pushButtonConfigureUT_clicked()
{
    DeviceConfig::ConfigureDevice(&unittester_device);
}

void SettingsOtherPeripherals::on_pushButtonConfigureKeyCard_clicked()
{
    DeviceConfig::ConfigureDevice(&novell_keycard_device);
}

void SettingsOtherPeripherals::on_checkBoxKeyCard_stateChanged(int arg1)
{
    ui->pushButtonConfigureKeyCard->setEnabled(arg1 != 0);
}

