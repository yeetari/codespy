#pragma once

#include <codespy/container/Vector.hh>

#include <QAbstractItemModel>

namespace codespy::gui {

struct ClassData {
    QString name;
    QString ir_text;
    QString bc_text;
};

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

private:
    Vector<ClassData> m_classes;

public:
    TreeModel(Vector<ClassData> &&classes);

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    const QString &ir_text(uint32_t index) const { return m_classes[index].ir_text; }
    const QString &bc_text(uint32_t index) const { return m_classes[index].bc_text; }
};

} // namespace codespy::gui
