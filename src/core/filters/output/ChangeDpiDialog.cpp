// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ChangeDpiDialog.h"

#include <QLineEdit>
#include <QMessageBox>

#include "PageSelectionAccessor.h"

namespace output {
ChangeDpiDialog::ChangeDpiDialog(QWidget* parent,
                                 const Dpi& dpi,
                                 const PageId& curPage,
                                 const PageSelectionAccessor& pageSelectionAccessor)
    : QDialog(parent),
      m_pages(pageSelectionAccessor.allPages()),
      m_selectedPages(pageSelectionAccessor.selectedPages()),
      m_curPage(curPage),
      m_scopeGroup(new QButtonGroup(this)) {
  setupUi(this);
  m_scopeGroup->addButton(thisPageRB);
  m_scopeGroup->addButton(allPagesRB);
  m_scopeGroup->addButton(thisPageAndFollowersRB);
  m_scopeGroup->addButton(selectedPagesRB);
  if (m_selectedPages.size() <= 1) {
    selectedPagesWidget->setEnabled(false);
  }

  dpiSelector->setValidator(new QIntValidator(dpiSelector));

  static const int common_dpis[] = {300, 400, 600};

  const int requestedDpi = std::max(dpi.horizontal(), dpi.vertical());
  m_customDpiString = QString::number(requestedDpi);

  int selectedIndex = -1;
  for (const int cdpi : common_dpis) {
    if (cdpi == requestedDpi) {
      selectedIndex = dpiSelector->count();
    }
    const QString cdpiStr(QString::number(cdpi));
    dpiSelector->addItem(cdpiStr, cdpiStr);
  }

  m_customItemIdx = dpiSelector->count();
  dpiSelector->addItem(tr("Custom"), m_customDpiString);

  if (selectedIndex != -1) {
    dpiSelector->setCurrentIndex(selectedIndex);
  } else {
    dpiSelector->setCurrentIndex(m_customItemIdx);
    dpiSelector->setEditable(true);
    dpiSelector->lineEdit()->setText(m_customDpiString);
    // It looks like we need to set a new validator
    // every time we make the combo box editable.
    dpiSelector->setValidator(new QIntValidator(0, 9999, dpiSelector));
  }

  connect(dpiSelector, SIGNAL(activated(int)), this, SLOT(dpiSelectionChanged(int)));
  connect(dpiSelector, SIGNAL(editTextChanged(const QString&)), this, SLOT(dpiEditTextChanged(const QString&)));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ChangeDpiDialog::~ChangeDpiDialog() = default;

void ChangeDpiDialog::dpiSelectionChanged(const int index) {
  dpiSelector->setEditable(index == m_customItemIdx);
  if (index == m_customItemIdx) {
    dpiSelector->setEditText(m_customDpiString);
    dpiSelector->lineEdit()->selectAll();
    // It looks like we need to set a new validator
    // every time we make the combo box editable.
    dpiSelector->setValidator(new QIntValidator(0, 9999, dpiSelector));
  }
}

void ChangeDpiDialog::dpiEditTextChanged(const QString& text) {
  if (dpiSelector->currentIndex() == m_customItemIdx) {
    m_customDpiString = text;
  }
}

void ChangeDpiDialog::onSubmit() {
  const QString dpiStr(dpiSelector->currentText());
  if (dpiStr.isEmpty()) {
    QMessageBox::warning(this, tr("Error"), tr("DPI is not set."));
    return;
  }

  const int dpi = dpiStr.toInt();
  if (dpi < 72) {
    QMessageBox::warning(this, tr("Error"), tr("DPI is too low!"));
    return;
  }

  if (dpi > 12800) {
    QMessageBox::warning(this, tr("Error"), tr("DPI is too high!"));
    return;
  }

  std::set<PageId> pages;

  if (thisPageRB->isChecked()) {
    pages.insert(m_curPage);
  } else if (allPagesRB->isChecked()) {
    m_pages.selectAll().swap(pages);
  } else if (thisPageAndFollowersRB->isChecked()) {
    m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
  } else if (selectedPagesRB->isChecked()) {
    emit accepted(m_selectedPages, Dpi(dpi, dpi));
    accept();
    return;
  }

  emit accepted(pages, Dpi(dpi, dpi));

  // We assume the default connection from accepted() to accept()
  // was removed.
  accept();
}  // ChangeDpiDialog::onSubmit
}  // namespace output
