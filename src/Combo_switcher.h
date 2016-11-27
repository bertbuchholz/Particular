#ifndef COMBO_SWITCHER_H
#define COMBO_SWITCHER_H

#include <QtGui>
#include <QWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>

class Combo_switcher : public QWidget
{
    Q_OBJECT
public:
    Combo_switcher()
    {
        /*
        QVBoxLayout * l = new QVBoxLayout;

        _combo_box = new QComboBox;
        _combo_box->addItem("A");
        _combo_box->addItem("B");
        _combo_box->addItem("C");

        connect(_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(change_sub_box(int)));

        l->addWidget(_combo_box);

        for (int i = 0; i < 3; ++i)
        {
            QGroupBox * b = new QGroupBox(QString("%1").arg(i));

            l->addWidget(b);

            _sub_boxes.push_back(b);
        }

        setLayout(l);

        change_sub_box(0);
        */
    }

    Combo_switcher(QComboBox * combo_box, std::vector<QGroupBox*> const& sub_boxes) :
        _combo_box(combo_box),
        _sub_boxes(sub_boxes)
    {
        QVBoxLayout * l = new QVBoxLayout;

        /*
        _combo_box = new QComboBox;
        _combo_box->addItem("A");
        _combo_box->addItem("B");
        _combo_box->addItem("C");
        */

        connect(_combo_box, SIGNAL(currentIndexChanged(int)), this, SLOT(change_sub_box(int)));

        l->addWidget(_combo_box);

        for (size_t i = 0; i < _sub_boxes.size(); ++i)
        {
            l->addWidget(_sub_boxes[i]);
        }

        setLayout(l);

        change_sub_box(combo_box->currentIndex());
    }

    ~Combo_switcher() {} // FIXME: members possibly have to be deleted

public slots:
    void change_sub_box(int const index)
    {
        for (size_t i = 0; i < _sub_boxes.size(); ++i)
        {
            _sub_boxes[i]->setVisible(false);
        }

        _sub_boxes[index]->setVisible(true);

        QWidget * w = this;

        while (w)
        {
            w->adjustSize();
            w = w->parentWidget();
        }
    }

private:
    QComboBox * _combo_box;
    std::vector<QGroupBox*> _sub_boxes;
};

#endif // COMBO_SWITCHER_H
