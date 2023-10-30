#include <codespy/gui/TreeModel.hh>

namespace codespy::gui {

TreeModel::TreeModel(Vector<ClassData> &&classes) : m_classes(std::move(classes)) {}

QVariant TreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return {};
    }
    return m_classes[index.internalId()].name;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Orientation::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    return "Classes";
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &) const {
    return createIndex(row, column, row);
}

QModelIndex TreeModel::parent(const QModelIndex &index) const {
    return QModelIndex();
}

int TreeModel::rowCount(const QModelIndex &parent) const {
    if (!parent.isValid()) {
        return m_classes.size();
    }
    return 0;
}

int TreeModel::columnCount(const QModelIndex &) const {
    return 1;
}

} // namespace codespy::gui
