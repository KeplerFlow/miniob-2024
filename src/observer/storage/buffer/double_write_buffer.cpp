/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wenbin1002 on 2024/04/16
//
#include <fcntl.h>

#include <mutex>
#include <algorithm>

#include "storage/buffer/double_write_buffer.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "common/io/io.h"
#include "common/log/log.h"
using namespace common;

struct DoubleWritePage
{
public:
  DoubleWritePage() = default;
  DoubleWritePage(int32_t buffer_pool_id, PageNum page_num, int32_t page_index, Page &page);

public:
  DoubleWritePageKey key;
  int32_t            page_index = -1; /// 页面在double write buffer文件中的页索引
  bool               valid = true; /// 表示页面是否有效，在页面被删除时，需要同时标记磁盘上的值。
  Page               page;

  static const int32_t SIZE;
};

DoubleWritePage::DoubleWritePage(int32_t buffer_pool_id, PageNum page_num, int32_t page_index, Page &_page)
  : key{buffer_pool_id, page_num}, page_index(page_index), page(_page)
{}

const int32_t DoubleWritePage::SIZE = sizeof(DoubleWritePage);

const int32_t DoubleWriteBufferHeader::SIZE = sizeof(DoubleWriteBufferHeader);

DiskDoubleWriteBuffer::DiskDoubleWriteBuffer(BufferPoolManager &bp_manager, int max_pages /*=16*/) 
  : max_pages_(max_pages), bp_manager_(bp_manager)
{
}

DiskDoubleWriteBuffer::~DiskDoubleWriteBuffer()
{
  flush_page();
  close(file_desc_);
}

RC DiskDoubleWriteBuffer::open_file(const char *filename)
{
  if (file_desc_ >= 0) {
    LOG_ERROR("Double write buffer has already opened. file desc=%d", file_desc_);
    return RC::BUFFERPOOL_OPEN;
  }
  
  int fd = open(filename, O_CREAT | O_RDWR, 0644);
  if (fd < 0) {
    LOG_ERROR("Failed to open or creat %s, due to %s.", filename, strerror(errno));
    return RC::SCHEMA_DB_EXIST;
  }

  file_desc_ = fd;
  return load_pages();
}

RC DiskDoubleWriteBuffer::flush_page()
{
  sync();

  for (const auto &pair : dblwr_pages_) {
    RC rc = write_page(pair.second);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    pair.second->valid = false;
    write_page_internal(pair.second);
    delete pair.second;
  }

  dblwr_pages_.clear();
  header_.page_cnt = 0;

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::add_page(DiskBufferPool *bp, PageNum page_num, Page &page)
{
  scoped_lock lock_guard(lock_);
  if (OB_FAIL(RC::SUCCESS)) {
    return RC::SUCCESS;
  }


  if (static_cast<int>(dblwr_pages_.size()) >= max_pages_) {
    RC rc = flush_page();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to flush pages in double write buffer");
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::write_page_internal(DoubleWritePage *page)
{
  int32_t page_index = page->page_index;
  int64_t offset = page_index * DoubleWritePage::SIZE + DoubleWriteBufferHeader::SIZE;
  if (lseek(file_desc_, offset, SEEK_SET) == -1) {
    LOG_ERROR("Failed to add page %lld of %d due to failed to seek %s.", offset, file_desc_, strerror(errno));
    return RC::IOERR_SEEK;
  }

  if (writen(file_desc_, page, DoubleWritePage::SIZE) != 0) {
    LOG_ERROR("Failed to add page %lld of %d due to %s.", offset, file_desc_, strerror(errno));
    return RC::IOERR_WRITE;
  }

  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::write_page(DoubleWritePage *dblwr_page)
{
  DiskBufferPool *disk_buffer = nullptr;
  // skip invalid page
  if (!dblwr_page->valid) {
    LOG_TRACE("double write buffer write page invalid. buffer_pool_id:%d,page_num:%d,lsn=%d",
              dblwr_page->key.buffer_pool_id, dblwr_page->key.page_num, dblwr_page->page.lsn);
    return RC::SUCCESS;
  }
  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::read_page(DiskBufferPool *bp, PageNum page_num, Page &page)
{

  return RC::BUFFERPOOL_INVALID_PAGE_NUM;
}

RC DiskDoubleWriteBuffer::clear_pages(DiskBufferPool *buffer_pool)
{
  vector<DoubleWritePage *> spec_pages;
  
 
  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::load_pages()
{
  if (file_desc_ < 0) {
    LOG_ERROR("Failed to load pages, due to file desc is invalid.");
    return RC::BUFFERPOOL_OPEN;
  }

  if (!dblwr_pages_.empty()) {
    LOG_ERROR("Failed to load pages, due to double write buffer is not empty. opened?");
    return RC::BUFFERPOOL_OPEN;
  }

  if (lseek(file_desc_, 0, SEEK_SET) == -1) {
    LOG_ERROR("Failed to load page header, due to failed to lseek:%s.", strerror(errno));
    return RC::IOERR_SEEK;
  }

  int ret = readn(file_desc_, &header_, sizeof(header_));
  if (ret != 0 && ret != -1) {
    LOG_ERROR("Failed to load page header, file_desc:%d, due to failed to read data:%s, ret=%d",
                file_desc_, strerror(errno), ret);
    return RC::IOERR_READ;
  }

  for (int page_num = 0; page_num < header_.page_cnt; page_num++) {
    int64_t offset = ((int64_t)page_num) * DoubleWritePage::SIZE + DoubleWriteBufferHeader::SIZE;

    if (lseek(file_desc_, offset, SEEK_SET) == -1) {
      LOG_ERROR("Failed to load page %d, offset=%ld, due to failed to lseek:%s.", page_num, offset, strerror(errno));
      return RC::IOERR_SEEK;
    }

    auto dblwr_page = make_unique<DoubleWritePage>();
    Page &page     = dblwr_page->page;
    ret = readn(file_desc_, dblwr_page.get(), DoubleWritePage::SIZE);
    if (ret != 0) {
      LOG_ERROR("Failed to load page, file_desc:%d, page num:%d, due to failed to read data:%s, ret=%d, page count=%d",
                file_desc_, page_num, strerror(errno), ret, page_num);
      return RC::IOERR_READ;
    }
  }

  LOG_INFO("double write buffer load pages done. page num=%d", dblwr_pages_.size());
  return RC::SUCCESS;
}

RC DiskDoubleWriteBuffer::recover()
{
  return flush_page();
}

