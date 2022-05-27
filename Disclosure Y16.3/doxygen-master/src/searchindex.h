/******************************************************************************
 *
 * Copyright (C) 1997-2020 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby
 * granted. No representations are made about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#ifndef SEARCHINDEX_H
#define SEARCHINDEX_H

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <array>
#include <functional>

#include "qcstring.h"

class Definition;

/*! Initialize the search indexer */
void initSearchIndexer();
/*! Cleanup the search indexer */
void finalizeSearchIndexer();

//------- server side search index ----------------------

struct URL
{
  URL(QCString n,QCString u) : name(n), url(u) {}
  QCString name;
  QCString url;
};


struct URLInfo
{
  URLInfo(int idx,int f) : urlIdx(idx), freq(f) {}
  int urlIdx;
  int freq;
};

class IndexWord
{
  public:
    using URLInfoMap = std::unordered_map<int,URLInfo>;
    IndexWord(QCString word);
    void addUrlIndex(int,bool);
    URLInfoMap urls() const { return m_urls; }
    QCString word() const { return m_word; }

  private:
    QCString    m_word;
    URLInfoMap  m_urls;
};

class SearchIndexIntf
{
  public:
    enum Kind { Internal, External };
    SearchIndexIntf(Kind k) : m_kind(k) {}
    virtual ~SearchIndexIntf() {}
    virtual void setCurrentDoc(const Definition *ctx,const QCString &anchor,bool isSourceFile) = 0;
    virtual void addWord(const QCString &word,bool hiPriority) = 0;
    virtual void write(const QCString &file) = 0;
    Kind kind() const { return m_kind; }
  private:
    Kind m_kind;
};

class SearchIndex : public SearchIndexIntf
{
  public:
    SearchIndex();
    void setCurrentDoc(const Definition *ctx,const QCString &anchor,bool isSourceFile) override;
    void addWord(const QCString &word,bool hiPriority) override;
    void write(const QCString &file) override;
  private:
    void addWord(const QCString &word,bool hiPrio,bool recurse);
    std::unordered_map<std::string,int> m_words;
    std::vector< std::vector< IndexWord> > m_index;
    std::unordered_map<std::string,int> m_url2IdMap;
    std::map<int,URL> m_urls;
    int m_urlIndex = -1;
};


class SearchIndexExternal : public SearchIndexIntf
{
    struct Private;
  public:
    SearchIndexExternal();
    void setCurrentDoc(const Definition *ctx,const QCString &anchor,bool isSourceFile);
    void addWord(const QCString &word,bool hiPriority);
    void write(const QCString &file);
  private:
    std::unique_ptr<Private> p;
};

//------- client side search index ----------------------

#define NUM_SEARCH_INDICES 21

QCString searchId(const Definition *d);
QCString searchName(const Definition *d);

using SearchIndexList = std::vector<const Definition *>;
using SearchIndexMap  = std::map<std::string,SearchIndexList>;

struct SearchIndexInfo
{
  void add(const std::string &letter,const Definition *def);
  QCString name;
  std::function<QCString()> getText;
  SearchIndexMap symbolMap;
};

void createJavaScriptSearchIndex();
void writeJavaScriptSearchIndex();
const std::array<SearchIndexInfo,NUM_SEARCH_INDICES> &getSearchIndices();

#endif
