/*
 * modbus.c
 *
 *  Created on: Jan 11, 2024
 *      Author: r6djo
 */

#include "modbus.h"

uint16_t MODBUS_CRC16(const uint8_t *nData, uint16_t wLength)
{
    union
    {
        uint8_t u8[2];
        uint16_t u16;
    } val;
    val.u16 = 0xFFFF;

    for (uint_least8_t i = 0; i < wLength; i++)
    { // до 255 элементов при подсчете CRC, иначе другой тип i
        uint_least8_t b = val.u8[0] ^ nData[i];
        val.u8[0] = val.u8[1];
        val.u8[1] = b;

        if ((val.u8[1] & 0x01) != 0)
        {
            val.u8[1] ^= 0x02;
            val.u8[0] ^= 0x40;
        }
        if ((val.u8[1] & 0x02) != 0)
        {
            val.u8[1] ^= 0x04;
            val.u8[0] ^= 0x80;
        }
        if ((val.u8[1] & 0x04) != 0)
            val.u8[1] ^= 0x09;
        if ((val.u8[1] & 0x08) != 0)
            val.u8[1] ^= 0x12;
        if ((val.u8[1] & 0x10) != 0)
            val.u8[1] ^= 0x24;
        if ((val.u8[1] & 0x20) != 0)
            val.u8[1] ^= 0x48;
        if ((val.u8[1] & 0x40) != 0)
            val.u8[1] ^= 0x90;
        if ((val.u8[1] & 0x80) != 0)
        {
            val.u8[1] ^= 0x20;
            val.u8[0] ^= 0x01;
        }
    }
    return val.u16;
}

uint16_t CRC16(const uint8_t *nData, uint16_t wLength)
{
    static const uint16_t wCRCTable[] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1,
        0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40,
        0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1,
        0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1,
        0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40,
        0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1,
        0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0,
        0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740,
        0xA501, 0x65C0, 0x6480, 0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0,
        0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1,
        0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1, 0x9181, 0x5140,
        0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0,
        0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0,
        0x4C80, 0x8C41, 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341,
        0x4100, 0x81C1, 0x8081, 0x4040};

    uint8_t nTemp;
    uint16_t wCRCWord = 0xFFFF;

    while (wLength--)
    {
        nTemp = *nData++ ^ wCRCWord;
        wCRCWord >>= 8;
        wCRCWord ^= wCRCTable[nTemp];
    }
    return wCRCWord;
}

uint8_t hex2int(char buf)
{
    if (buf >= '0' && buf <= '9')
        return buf - '0';
    if (buf >= 'a' && buf <= 'f')
        return buf - 'a' + 10;
    if (buf >= 'A' && buf <= 'F')
        return buf - 'A' + 10;
    return 0;
}

modbus_status_t msg_validate(UART_message *msg)
{
    if (msg->msg_length < 4)
    {
        return MB_ERR;
    }
    uint16_t crc = MODBUS_CRC16(msg->msg_data, msg->msg_length - 2);
    uint16_t msg_crc = (msg->msg_data[msg->msg_length - 2]) | (msg->msg_data[msg->msg_length - 1] << 8);
    if (crc != msg_crc)
    {
        return MB_ERR;
    }
    return MB_OK;
}

modbus_status_t msg_parse(UART_message *buf, MODBUS_message *rx_msg)
{
    rx_msg->device_address = buf->msg_data[0];
    rx_msg->command = buf->msg_data[1];

    switch (rx_msg->command)
    {
    case READ_DO:
    case READ_DI:
    case READ_AO:
    case READ_AI:
        rx_msg->start_address = (buf->msg_data[2] << 8) | buf->msg_data[3];
        rx_msg->data_length = (buf->msg_data[4] << 8) | buf->msg_data[5];
        break;
    case WRITE_DO:
    case WRITE_AO:
        rx_msg->start_address = (buf->msg_data[2] << 8) | buf->msg_data[3];
        rx_msg->data[0] = (buf->msg_data[4] << 8) | buf->msg_data[5];
        rx_msg->data_length = 0x01;
        break;
    case READ_EXCEPTION:
    case DIAGNOSTIC:
        break;
    case WRITE_DO_MULTI:
    case WRITE_AO_MULTI:
        rx_msg->start_address = (buf->msg_data[2] << 8) | buf->msg_data[3];
        rx_msg->data_length = (buf->msg_data[4] << 8) | buf->msg_data[5];
        rx_msg->byte_count = buf->msg_data[6];
        for (uint8_t i = 0; i < buf->msg_length - 9; i++)
        {
            rx_msg->data[i] = (buf->msg_data[7 + i * 2] << 8) | buf->msg_data[8 + i * 2];
        }
        break;
    default:
        break;
    }

    rx_msg->crc = (buf->msg_data[buf->msg_length - 2]) | (buf->msg_data[buf->msg_length - 1] << 8);
    return MB_OK;
}

modbus_status_t address_validate(MODBUS_message *rx_msg, MODBUS_registers *registers)
{
    switch (rx_msg->command)
    {
    case READ_DO:
        if (rx_msg->start_address >= registers->DO_start_address &&
            rx_msg->start_address + rx_msg->data_length < registers->DO_start_address + registers->DO_count)
        {
            return MB_OK;
        }
        break;
    case READ_DI:
        if (rx_msg->start_address >= registers->DI_start_address &&
            rx_msg->start_address + rx_msg->data_length < registers->DI_start_address + registers->DI_count)
        {
            return MB_OK;
        }
        break;
    case READ_AO:
        if (rx_msg->start_address >= registers->AO_start_address &&
            rx_msg->start_address + rx_msg->data_length < registers->AO_start_address + registers->AO_count)
        {
            return MB_OK;
        }
        break;
    case READ_AI:
        if (rx_msg->start_address >= registers->AI_start_address &&
            rx_msg->start_address + rx_msg->data_length < registers->AI_start_address + registers->AI_count)
        {
            return MB_OK;
        }
        break;
    case WRITE_DO:
        if (rx_msg->start_address >= registers->DO_start_address &&
            rx_msg->start_address < registers->DO_start_address + registers->DO_count)
        {
            return MB_OK;
        }
        break;
    case WRITE_AO:
        if (rx_msg->start_address >= registers->AO_start_address &&
            rx_msg->start_address < registers->AO_start_address + registers->AO_count)
        {
            return MB_OK;
        }
        break;
    case READ_EXCEPTION:
        return MB_OK;
        break;
    case DIAGNOSTIC:
        return MB_OK;
        break;
    case WRITE_DO_MULTI:
        if (rx_msg->start_address >= registers->DO_start_address &&
            rx_msg->start_address + rx_msg->data_length < registers->DO_start_address + registers->DO_count)
        {
            return MB_OK;
        }
        break;
    case WRITE_AO_MULTI:
        if (rx_msg->start_address >= registers->AO_start_address &&
            rx_msg->start_address + rx_msg->data_length < registers->AO_start_address + registers->AO_count)
        {
            return MB_OK;
        }
        break;
    default:
        return MB_OK;
    }
    return MB_ERR;
}

modbus_status_t response_prepare(MODBUS_message *rx_msg, MODBUS_registers *registers, UART_message *tx_buf)
{
    uint16_t shift;
    if (rx_msg->device_address != registers->MB_address)
    {
        return WRONG_ADDRESS;
    }
    tx_buf->msg_length = 0;
    modbus_status_t adreess_is_valid = address_validate(rx_msg, registers);
    if (adreess_is_valid)
    {
        tx_buf->msg_data[tx_buf->msg_length++] = registers->MB_address;
        tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->command;
        switch (rx_msg->command)
        {
        case READ_DO:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length << 1;
            for (uint16_t i = 0; i < rx_msg->data_length; i++)
            {
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->DO[rx_msg->start_address - registers->DO_start_address + i] >> 8;
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->DO[rx_msg->start_address - registers->DO_start_address + i] & 0xFF;
            }
            break;
        case READ_DI:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length << 1;
            for (uint16_t i = 0; i < rx_msg->data_length; i++)
            {
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->DI[rx_msg->start_address - registers->DO_start_address] >> 8;
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->DI[rx_msg->start_address - registers->DO_start_address] & 0xFF;
            }
            break;
        case READ_AO:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length << 1;
            for (uint16_t i = 0; i < rx_msg->data_length; i++)
            {
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->AO[rx_msg->start_address - registers->AO_start_address] >> 8;
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->AO[rx_msg->start_address - registers->AO_start_address] & 0xFF;
            }
            break;
        case READ_AI:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length << 1;
            for (uint16_t i = 0; i < rx_msg->data_length; i++)
            {
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->AI[rx_msg->start_address - registers->AI_start_address] >> 8;
                tx_buf->msg_data[tx_buf->msg_length++] =
                    registers->AI[rx_msg->start_address - registers->AI_start_address] & 0xFF;
            }
            break;
        case WRITE_DO:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->start_address >> 8;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->start_address & 0xFF;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data[0] >> 8;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data[0] & 0xFF;
            shift = rx_msg->start_address - registers->DO_start_address;
            registers->DO[shift] = rx_msg->data[0];
            break;
        case WRITE_AO:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->start_address >> 8;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->start_address & 0xFF;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data[0] >> 8;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data[0] & 0xFF;
            shift = rx_msg->start_address - registers->DO_start_address;
            registers->AO[shift] = rx_msg->data[0];
            break;
        case READ_EXCEPTION:
            break;
        case DIAGNOSTIC:
            break;
        case WRITE_DO_MULTI:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length >> 8;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length & 0xFF;
            shift = rx_msg->start_address - registers->DO_start_address;
            for (uint16_t i = 0; i < rx_msg->data_length; i++)
            {
                registers->DO[shift + i] = rx_msg->data[i];
            }
            break;
        case WRITE_AO_MULTI:
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length >> 8;
            tx_buf->msg_data[tx_buf->msg_length++] = rx_msg->data_length & 0xFF;
            shift = rx_msg->start_address - registers->AO_start_address;
            for (uint16_t i = 0; i < rx_msg->data_length; i++)
            {
                registers->AO[shift + i] = rx_msg->data[i];
            }
            break;
        default:
            tx_buf->msg_data[1] = rx_msg->command | 0x80;
            tx_buf->msg_data[2] = 0x01;
            tx_buf->msg_length = 3;
        }
    }
    else
    {
        switch (rx_msg->command)
        {
        case READ_DO:
        case READ_DI:
        case READ_AO:
        case READ_AI:
        case WRITE_DO:
        case WRITE_AO:
        case WRITE_DO_MULTI:
        case WRITE_AO_MULTI:
            tx_buf->msg_data[1] = rx_msg->command | 0x80;
            tx_buf->msg_data[2] = 0x02;
            tx_buf->msg_length = 3;
        }
    }

    uint16_t crc = MODBUS_CRC16(tx_buf->msg_data, tx_buf->msg_length);
    tx_buf->msg_data[tx_buf->msg_length++] = crc & 0xFF;
    tx_buf->msg_data[tx_buf->msg_length++] = crc >> 8;

    return MB_OK;
}

modbus_status_t prepare_request_registers(uint8_t device_address, uint8_t command, uint16_t start_address,
                                          uint16_t count, UART_message *tx_buf)
{
    tx_buf->msg_length = 0;
    tx_buf->msg_data[tx_buf->msg_length++] = device_address;
    tx_buf->msg_data[tx_buf->msg_length++] = command;
    tx_buf->msg_data[tx_buf->msg_length++] = start_address >> 8;
    tx_buf->msg_data[tx_buf->msg_length++] = start_address & 0xFF;
    tx_buf->msg_data[tx_buf->msg_length++] = count >> 8;
    tx_buf->msg_data[tx_buf->msg_length++] = count & 0xFF;
    uint16_t crc = MODBUS_CRC16(tx_buf->msg_data, tx_buf->msg_length);
    tx_buf->msg_data[tx_buf->msg_length++] = crc & 0xFF;
    tx_buf->msg_data[tx_buf->msg_length++] = crc >> 8;
    return MB_OK;
}

modbus_status_t prepare_request_registers(MODBUS_message *request, UART_message *tx_buf)
{
    tx_buf->msg_length = 0;
    tx_buf->msg_data[tx_buf->msg_length++] = request->device_address;
    tx_buf->msg_data[tx_buf->msg_length++] = request->command;
    tx_buf->msg_data[tx_buf->msg_length++] = request->start_address >> 8;
    tx_buf->msg_data[tx_buf->msg_length++] = request->start_address & 0xFF;
    tx_buf->msg_data[tx_buf->msg_length++] = request->data_length >> 8;
    tx_buf->msg_data[tx_buf->msg_length++] = request->data_length & 0xFF;
    uint16_t crc = MODBUS_CRC16(tx_buf->msg_data, tx_buf->msg_length);
    tx_buf->msg_data[tx_buf->msg_length++] = crc & 0xFF;
    tx_buf->msg_data[tx_buf->msg_length++] = crc >> 8;
    return MB_OK;
}

modbus_status_t response_processing(MODBUS_message *response, MODBUS_message *request, MODBUS_registers *registers)
{
    // AI[0] - current light level
    // AO[1] - light threshold
    // DO[0].0 - mode 1-automatic/0-manual
    // DO[1].0 - light 1-on/0-off
    if(response->device_address!=request->device_address)
    {
        return WRONG_ADDRESS;
    }
    if(response->command!=request->command)
    {
        return WRONG_COMMAND;
    }
    return MB_OK;
}

//
// uint8_t prepare_tx_msg(union message *rx_msg, union message *tx_msg,
// uint16_t *data)
//{
//    if (rx_msg->msg.device_address != DEVICE_ADDRESS)
//        return 0;
//    tx_msg->msg.device_address = rx_msg->msg.device_address;
//
//    if (rx_msg->msg.command != COMMAND)
//        return 0;
//    tx_msg->msg.command = rx_msg->msg.command;
//
//    if (rx_msg->msg.start_address < START_ADDRESS ||
//    (rx_msg->msg.start_address + rx_msg->msg.data_length) > START_ADDRESS +
//    REGISTER_COUNT)
//        return 0;
//
//    tx_msg->msg.start_address = rx_msg->msg.start_address;
//    tx_msg->msg.data_length = rx_msg->msg.data_length;
//
//    uint8_t offset = tx_msg->msg.start_address - START_ADDRESS;
//    for (uint8_t i = 0; i < tx_msg->msg.data_length; i++)
//    {
//        tx_msg->msg.data[i] = data[i + offset];
//    }
//    return 1;
//}
//
// char *itoalz(char *buf, uint8_t value)
//{
//	char tmp[3];
//    if (value <= 0x0F)
//        strcat(buf, "0");
//    strcat(buf, itoa(value, tmp, 16));
//    return buf;
//}
//
// void prepare_tx_buf(union message *tx_msg, uint8_t *tx_buf)
//{
//    strcat(tx_buf, ":");
//
//    itoalz(tx_buf, tx_msg->msg.device_address);
//
//    itoalz(tx_buf, tx_msg->msg.command);
//
//    itoalz(tx_buf, tx_msg->msg.start_address >> 8);
//    itoalz(tx_buf, tx_msg->msg.start_address & 0xFF);
//
//    itoalz(tx_buf, tx_msg->msg.data_length >> 8);
//    itoalz(tx_buf, tx_msg->msg.data_length & 0xFF);
//
//    for (uint8_t i = 0; i < tx_msg->msg.data_length; i++)
//    {
//        itoalz(tx_buf, tx_msg->msg.data[i] >> 8);
//        itoalz(tx_buf, tx_msg->msg.data[i] & 0xFF);
//    }
//    uint8_t checksum = 0;
//    checksum += tx_msg->msg.device_address;
//
//    checksum += tx_msg->msg.command;
//
//    checksum += tx_msg->msg.start_address >> 8;
//    checksum += tx_msg->msg.start_address & 0xFF;
//
//    checksum += tx_msg->msg.data_length >> 8;
//    checksum += tx_msg->msg.data_length & 0xFF;
//
//    for (uint8_t i = 0; i < tx_msg->msg.data_length; i++)
//    {
//        checksum += (tx_msg->msg.data[i] >> 8);
//        checksum += (tx_msg->msg.data[i] & 0xFF);
//    }
//    checksum = 255 - checksum + 1;
//
//    itoalz(tx_buf, checksum);
//    strcat(tx_buf, "\r\n");
//}
