/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "js_buffer.h"
#include "securec.h"

using namespace std;

namespace OHOS::buffer {
void Buffer::Init(uint32_t size)
{
    if (size > 0) {
        raw_ = reinterpret_cast<uint8_t *>(malloc(size));
        if (raw_ == nullptr) {
            HILOG_FATAL("Buffer:: constructor malloc failed");
            length_ = 0;
        }
    }
    length_ = size;
}

void Buffer::Init(Buffer *buffer)
{
    if (buffer != nullptr && buffer->length_ > 0) {
        this->raw_ = reinterpret_cast<uint8_t *>(malloc(buffer->length_));
        if (raw_ == nullptr) {
            HILOG_FATAL("Buffer:: constructor malloc failed");
        } else {
            this->length_ = buffer->length_;
            if (memcpy_s(raw_, length_, buffer->raw_ + buffer->byteOffset_, length_) != EOK) {
                HILOG_FATAL("Buffer:: constructor memcpy_s failed");
            }
        }
    }
}

void Buffer::Init(Buffer *pool, unsigned int poolOffset, unsigned int length)
{
    if (pool != nullptr) {
        this->raw_ = pool->GetRaw();
        this->byteOffset_ = poolOffset;
        this->length_ = length;
    }
    this->needRelease_ = false;
}

void Buffer::Init(uint8_t *buffer, unsigned int byteOffset, unsigned int length)
{
    if (buffer != nullptr) {
        this->raw_ = buffer;
        this->length_ = length;
        this->byteOffset_ = byteOffset;
    }
    this->needRelease_ = false;
}

Buffer::~Buffer()
{
    if (raw_ != nullptr && needRelease_) {
        free(raw_);
        raw_ = nullptr;
    }
}

EncodingType Buffer::GetEncodingType(std::string encode)
{
    if (encode == "hex") {
        return HEX;
    } else if (encode == "base64url") {
        return BASE64URL;
    } else if (encode == "ascii") {
        return ASCII;
    } else if (encode == "base64") {
        return BASE64;
    } else if (encode == "latin1") {
        return LATIN1;
    } else if (encode == "binary") {
        return BINARY;
    } else if (encode == "utf16le") {
        return UTF16LE;
    } else {
        return UTF8;
    }
}

uint8_t *Buffer::GetRaw()
{
    return raw_;
}

unsigned int Buffer::GetLength()
{
    return length_;
}

void Buffer::SetLength(unsigned int len)
{
    length_ = len;
}

unsigned int Buffer::GetByteOffset()
{
    return byteOffset_;
}

void Buffer::SubBuffer(Buffer *tBuf, uint32_t start, uint32_t end)
{
    if (tBuf == nullptr) {
        HILOG_ERROR("SubBuffer: buffer is nullptr.");
        return;
    }
    this->Init(tBuf->GetRaw(), tBuf->byteOffset_ + start, (end - start));
}

uint32_t Buffer::Copy(Buffer *tBuf, uint32_t tStart, uint32_t sStart, uint32_t sEnd)
{
    if (tBuf == nullptr) {
        return 0; // 0 : cannot copy anything
    }
    uint8_t *dest = tBuf->raw_ + tBuf->byteOffset_ + tStart;
    uint32_t tLength = tBuf->length_ - tStart;
    uint8_t *src = this->raw_ + this->byteOffset_ + sStart;
    uint32_t sLength = sEnd - sStart;
    uint32_t len = tLength > sLength ? sLength : tLength;
    if (tBuf->raw_ == this->raw_) {
        if (src == nullptr || dest == nullptr) {
            return len;
        }
        if (len == 0) {
            HILOG_DEBUG("Buffer:: WriteBytes size is 0");
            return len;
        }
        if (memmove_s(dest, len, src, len) != EOK) {
            HILOG_FATAL("Buffer:: WriteBytes memmove_s failed");
            return len;
        }
    } else {
        WriteBytes(src, len, dest);
    }
    return len;
}

int Buffer::Compare(Buffer *tBuf, uint32_t targetStart, uint32_t sourceStart, uint32_t length)
{
    if (tBuf == nullptr) {
        return 0;
    }
    uint8_t *dest = tBuf->GetRaw() + tBuf->byteOffset_ + targetStart;
    uint8_t *src = this->GetRaw() + this->byteOffset_ + sourceStart;
    return memcmp(dest, src, length);
}

void Buffer::WriteBE(int32_t value, uint32_t bytes)
{
    uint32_t uValue = static_cast<uint32_t>(value);
    for (uint32_t i = bytes; i > 0; i--) {
        uint8_t va = static_cast<uint8_t>(uValue & 0x000000FF);
        data_[i - 1] = va;
        // 8 : shift right 8 bits(i.e 1 byte)
        uValue = uValue >> 8;
    }
}

void Buffer::WriteLE(int32_t value, uint32_t bytes)
{
    uint32_t uValue = static_cast<uint32_t>(value);
    for (uint32_t i = 0, len = bytes - 1; i <= len; i++) {
        uint8_t va = static_cast<uint8_t>(uValue & 0x000000FF);
        data_[i] = va;
        // 8 : shift right 8 bits(i.e 1 byte)
        uValue = uValue >> 8;
    }
}

uint32_t Buffer::ReadBE(uint32_t bytes)
{
    uint32_t result = 0x0000;
    for (uint32_t i = 0; i < bytes; i++) {
        // 8 : shift left 8 bits(i.e 1 byte)
        result = result << 8;
        result |= data_[i];
    }
    return result;
}

uint32_t Buffer::ReadLE(uint32_t bytes)
{
    uint32_t result = 0x0000;
    for (uint32_t i = bytes; i > 0; i--) {
        // 8 : shift left 8 bits(i.e 1 byte)
        result = result << 8;
        result |= data_[i - 1];
    }
    return result;
}

void Buffer::WriteInt32BE(int32_t value, uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    WriteBE(value, 4);
    // 4 : write 4 bytes
    WriteBytes(data_, 4, raw_ + byteOffset_ + offset);
}

int32_t Buffer::ReadInt32BE(uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    ReadBytes(data_, offset, 4);
    // 4 : read 4 bytes
    return static_cast<int32_t>(ReadBE(4));
}

void Buffer::WriteInt32LE(int32_t value, uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    WriteLE(value, 4);
    // 4 : write 4 bytes
    WriteBytes(data_, 4, raw_ + byteOffset_ + offset);
}

int32_t Buffer::ReadInt32LE(uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    ReadBytes(data_, offset, 4);
    // 4 : read 4 bytes
    return static_cast<int32_t>(ReadLE(4));
}

void Buffer::WriteUInt32BE(int32_t value, uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    WriteBE(value, 4);
    // 4 : write 4 bytes
    WriteBytes(data_, 4, raw_ + byteOffset_ + offset);
}

uint32_t Buffer::ReadUInt32BE(uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    ReadBytes(data_, offset, 4);
    // 4 : read 4 bytes
    return ReadBE(4);
}

void Buffer::WriteUInt32LE(int32_t value, uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    WriteLE(value, 4);
    // 4 : write 4 bytes
    WriteBytes(data_, 4, raw_ + byteOffset_ + offset);
}

uint32_t Buffer::ReadUInt32LE(uint32_t offset)
{
    // 4 : 4 bytes(i.e 4 * 8 = 32 bits)
    ReadBytes(data_, offset, 4);
    // 4 : read 4 bytes
    return ReadLE(4);
}

int32_t Buffer::Get(uint32_t index)
{
    uint8_t value;
    uint32_t count = 1;
    if (memcpy_s(&value, count, raw_ + byteOffset_ + index, count) != EOK) {
        HILOG_FATAL("Get:: get memcpy_s failed");
    }
    return value;
}

void Buffer::Set(uint32_t index, uint8_t value)
{
    WriteByte(value, index);
}

void Buffer::ReadBytes(uint8_t *data, uint32_t offset, uint32_t length)
{
    if (data == nullptr) {
        return;
    }

    if (length == 0) {
        HILOG_DEBUG("Buffer:: ReadBytes size is 0");
        return;
    }
    if (memcpy_s(data, length, raw_ + byteOffset_ + offset, length) != EOK) {
        HILOG_FATAL("Buffer:: ReadBytes memcpy_s failed");
    }
}

void Buffer::ReadBytesForArrayBuffer(void *data, uint32_t length)
{
    if (length == 0) {
        HILOG_DEBUG("Buffer:: ReadBytesForArrayBuffer size is 0");
        return;
    }
    if (memcpy_s(data, length, reinterpret_cast<const void*>(raw_ + byteOffset_), length) != EOK) {
        HILOG_ERROR("Buffer:: copy raw to arraybuffer error");
        return;
    }
}

void Buffer::WriteByte(uint8_t number, uint32_t offset)
{
    WriteBytes(&number, 1, raw_ + byteOffset_ + offset);
}

unsigned int Buffer::WriteString(std::string value, unsigned int size)
{
    uint8_t *str = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(value.data()));
    bool isWriteSuccess = WriteBytes(str, size, raw_ + byteOffset_);
    return isWriteSuccess ? size : 0; // 0: write failed
}

unsigned int Buffer::WriteString(string value, unsigned int offset, unsigned int length)
{
    uint8_t *str = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(value.data()));
    bool isWriteSuccess = WriteBytes(str, length, raw_ + byteOffset_ + offset);
    return isWriteSuccess ? length : 0; // 0: write failed
}

void Buffer::WriteStringLoop(string value, unsigned int offset, unsigned int end, unsigned int length)
{
    if (end - offset <= 0 || value.length() == 0) {
        return;
    }
    unsigned int loop = length > end - offset ? end - offset : length;

    uint8_t *str = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(value.data()));
    while (offset < end) {
        if (loop + offset > end) {
            WriteBytes(str, end - offset, raw_ + byteOffset_ + offset);
        } else {
            WriteBytes(str, loop, raw_ + byteOffset_ + offset);
        }
        offset += loop;
    }
}

std::string Buffer::Utf16StrToStr(std::u16string value)
{
    string str = "";
    char16_t *data = const_cast<char16_t *>(reinterpret_cast<const char16_t *>(value.data()));
    for (unsigned int i = 0; i < value.length(); i++) {
        char16_t c = data[i];
        char high = static_cast<char>((c >> 8) & 0x00FF);
        char low = static_cast<char>(c & 0x00FF);
        str.push_back(low);
        str.push_back(high);
    }
    return str;
}

unsigned int Buffer::WriteString(std::u16string value, unsigned int offset, unsigned int length)
{
    string str = Utf16StrToStr(value);
    return this->WriteString(str, offset, length);
}

void Buffer::WriteStringLoop(std::u16string value, unsigned int offset, unsigned int end)
{
    string str = Utf16StrToStr(value);
    // 2 : utf16 is 2 times of length of ascii
    this->WriteStringLoop(str, offset, end, value.length() * 2);
}

bool Buffer::WriteBytes(uint8_t *src, unsigned int size, uint8_t *dest)
{
    if (src == nullptr || dest == nullptr) {
        return false;
    }
    if (size == 0) {
        HILOG_DEBUG("Buffer:: WriteBytes size is 0");
        return false;
    }
    if (memcpy_s(dest, size, src, size) != EOK) {
        HILOG_FATAL("Buffer:: WriteBytes memcpy_s failed");
        return false;
    }
    return true;
}

void Buffer::SetArray(vector<uint8_t> array, unsigned int offset)
{
    unsigned int arrLen = array.size();
    unsigned int size = arrLen <= length_ ? arrLen : length_;
    WriteBytes(array.data(), size, raw_ + byteOffset_ + offset);
}

void Buffer::FillBuffer(Buffer *buffer, unsigned int offset, unsigned int end)
{
    if (buffer == nullptr) {
        return;
    }
    if (end - offset <= 0) {
        return;
    }
    unsigned int loop = buffer->GetLength() > end - offset ? end - offset : buffer->GetLength();

    while (offset < end) {
        if (offset + loop > end) {
            loop = end - offset;
        }
        WriteBytes(buffer->GetRaw() + buffer->byteOffset_, loop, raw_ + byteOffset_ + offset);
        offset += loop;
    }
}

void Buffer::FillNumber(vector<uint8_t> numbers, unsigned int offset, unsigned int end)
{
    if (end - offset <= 0) {
        return;
    }
    unsigned int loop = numbers.size() > end - offset ? end - offset : numbers.size();

    while (offset < end) {
        if (offset + loop > end) {
            loop = end - offset;
        }
        WriteBytes(numbers.data(), loop, raw_ + byteOffset_ + offset);
        offset += loop;
    }
}

std::string Buffer::GetString(std::string value, EncodingType encodingType)
{
    string str = "";
    switch (encodingType) {
        case ASCII:
        case LATIN1:
            str = Utf8ToUtf16BEToANSI(value);
            break;
        case UTF8:
            str = value;
            break;
        case BASE64:
        case BASE64URL:
            str = Base64Decode(value, encodingType);
            break;
        case BINARY:
            str = Utf8ToUtf16BEToANSI(value);
            break;
        default:
            break;
    }
    return str;
}

void Buffer::FillString(string value, unsigned int offset, unsigned int end, string encoding)
{
    EncodingType encodingType = GetEncodingType(encoding);
    if (encodingType == UTF16LE) {
        u16string u16Str = Utf8ToUtf16BE(value);
        this->WriteStringLoop(u16Str, offset, end);
    } else {
        string str = GetString(value, encodingType);
        this->WriteStringLoop(str, offset, end, str.length());
    }
}

unsigned int Buffer::WriteString(string value, unsigned int offset, unsigned int length, string encoding)
{
    EncodingType encodingType = GetEncodingType(encoding);
    unsigned int lengthWrote = 0;
    if (encodingType == UTF16LE) {
        u16string u16Str = Utf8ToUtf16BE(value);
        lengthWrote = this->WriteString(u16Str, offset, length);
    } else {
        string str = GetString(value, encodingType);
        lengthWrote = this->WriteString(str, offset, length);
    }
    return lengthWrote;
}

std::string Buffer::ToBase64(uint32_t start, uint32_t length)
{
    if (length == 0 || length >= UINT32_MAX) {
        HILOG_ERROR("Buffer:: length is illegal");
        return "";
    }
    uint8_t *data = new (std::nothrow) uint8_t[length];
    if (data == nullptr) {
        HILOG_ERROR("Buffer:: memory allocation failed.");
        return "";
    }
    ReadBytes(data, start, length);
    std::string result = Base64Encode(reinterpret_cast<const unsigned char*>(data), length, BASE64);
    delete[] data;
    data = nullptr;
    return result;
}

std::string Buffer::ToBase64Url(uint32_t start, uint32_t length)
{
    if (length == 0 || length >= UINT32_MAX) {
        HILOG_ERROR("Buffer:: length is illegal");
        return "";
    }
    uint8_t *data = new (std::nothrow) uint8_t[length];
    if (data == nullptr) {
        HILOG_ERROR("Buffer:: memory allocation failed.");
        return "";
    }
    ReadBytes(data, start, length);
    std::string result = Base64Encode(reinterpret_cast<const unsigned char*>(data), length, BASE64URL);
    delete[] data;
    data = nullptr;
    return result;
}

int Buffer::IndexOf(const char *data, uint32_t offset, uint32_t len, uint64_t &resultIndex)
{
    if (data == nullptr || length_ <= offset) {
        return -1;
    }
    uint8_t *sData = new (std::nothrow) uint8_t[length_ - offset];
    if (sData == nullptr) {
        HILOG_ERROR("Buffer:: memory allocation failed.");
        return -1;
    }
    ReadBytes(sData, offset, length_ - offset);
    int index = FindIndex(sData, reinterpret_cast<uint8_t *>(const_cast<char *>(data)), length_ - offset, len);
    if (index == -1) { // -1:The target to be searched does not exist
        delete[] sData;
        sData = nullptr;
        return index;
    } else {
        resultIndex = static_cast<uint64_t>(offset) + static_cast<uint64_t>(index);
        delete[] sData;
        sData = nullptr;
        return -2; // -2:The number of invalid data
    }
}

int Buffer::LastIndexOf(const char *data, uint32_t offset, uint32_t len)
{
    if (data == nullptr || length_ <= offset) {
        return -1;
    }
    uint8_t *sData = new (std::nothrow) uint8_t[length_ - offset];
    if (sData == nullptr) {
        HILOG_ERROR("Buffer:: memory allocation failed.");
        return -1;
    }
    ReadBytes(sData, offset, length_ - offset);
    int result = FindLastIndex(sData, reinterpret_cast<uint8_t *>(const_cast<char *>(data)), length_ - offset, len);
    delete[] sData;
    sData = nullptr;
    return result;
}
} // namespace OHOS::Buffer
