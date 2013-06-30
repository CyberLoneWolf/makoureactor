#include "OperationsManager.h"

OperationsManager::OperationsManager(bool isPC, QWidget *parent) :
	QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	setWindowTitle(tr("Op�rations diverses"));

	addOperation(CleanUnusedTexts, tr("Effacer tous les textes inutilis�s"));
	addOperation(RemoveTexts, tr("D�sactiver tous les textes du jeu"));
	addOperation(RemoveBattles, tr("D�sactiver tous combats du jeu"));
	if(isPC) {
		addOperation(CleanModelLoaderPC, tr("Supprimer les donn�es inutiles des listes des mod�les 3D"));
		addOperation(RemoveUnusedSectionPC, tr("Supprimer les donn�es inutilis�es pour les d�cors"));
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel);

	QVBoxLayout *layout = new QVBoxLayout(this);
	foreach(QCheckBox *operation, _operations) {
		layout->addWidget(operation);
	}
	layout->addStretch();
	layout->addWidget(buttonBox);

	connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void OperationsManager::addOperation(Operation op, const QString &description)
{
	_operations.insert(op, new QCheckBox(description, this));
}

OperationsManager::Operations OperationsManager::selectedOperations() const
{
	QMapIterator<Operation, QCheckBox *> it(_operations);
	Operations ret = Operations();

	while(it.hasNext()) {
		it.next();

		if(it.value()->isChecked()) {
			ret |= it.key();
		}
	}

	return ret;
}
