/****************************************************************************
**
** A tool for sharing feedback on the Kano OS
**
****************************************************************************/

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTextEdit>
#include <QFont>
#include <QString>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QComboBox>
#include <QImage>
#include <QInputDialog>

#include <string.h>
#include <pwd.h>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "include/mainwindow.h"

#define WIDTH     500

/******************* Styling *******************
 *                                             *
 * TODO: Move away from using the preprocessor *
 *                                             *
 ***********************************************/

/* General App Style */
#define APP_STYLING  " \
QWidget { \
    background-color: #ffffff; \
     \
}" //background-color: #d1d2d4;

/* The main heading label */
#define HEADING_LABEL_STYLING  " \
QLabel { \
    background-color: #ffffff; \
    color: #000000; \
    margin-top: 2px; \
    margin-bottom: 0; \
    padding: 8px; \
    font-family: 'Bariol'; \
    font-size: 35px; \
}"

/* The label describing the output pane */
#define LABEL_STYLING  " \
QLabel { \
    background-color: #ffffff; \
    color: #000000; \
    margin-top: 2px; \
    margin-bottom: 0; \
    padding: 8px; \
    font-family: 'Bariol'; \
    font-size: 18px; \
}"

/* The pane showing the output */
#define FEEDBACK_TEXT_PANE_STYLING  " \
QTextEdit { \
    background-color: #ffffff; \
    margin: 0; \
    font-family: 'Bariol'; \
    font-size: 30px; \
}"

#define FEEDBACK_CATEGORY_PANE_STYLING  " \
QComboBox { \
    background-color: #ffffff; \
    margin: 0; \
    padding: 5px 20px; \
    font-family: 'Bariol'; \
    height: 30px; \
    font-size: 18px; \
    color: #636466; \
}"

#define SUBMIT_BUTTON_STYLING  " \
QPushButton { \
    background-color: #ffa53b; \
    margin: 0; \
    font-family: 'Bariol'; \
    font-size: 18px; \
    color: #ffffff; \
}"


/***** End *****
 *      of     *
 *** styling ***/


/***********************************************************/
/*********************** Constructor ***********************/
/***********************************************************/

MainWindow::MainWindow(QApplication &app)
{
  /* Close button */
  QPushButton * CloseButton = new QPushButton("", this);
  CloseButton->setFlat(true);
  CloseButton->setIcon(QPixmap( ":resources/close.png" ));
  CloseButton->setMinimumWidth(20);

  // Connect button signal to appropriate slot
  connect(CloseButton, SIGNAL(released()), this, SLOT(onExit()));


  /* Main heading */
  QLabel * feedbackLabel = new QLabel();
  feedbackLabel->setText("Send feedback to the Kano Team!");
  feedbackLabel->setStyleSheet(HEADING_LABEL_STYLING);

  /* Explanation heading */
  QLabel * explanationLabel = new QLabel();
  explanationLabel->setText("Found bugs or just like what we've done?");
  explanationLabel->setStyleSheet(LABEL_STYLING);

  /* Feedback category */
  categories << "Subject" << "--------------------------------------------------------" << "Comment" << "Bug" << "Suggestions" << "Question";

  FeedbackCategoryDropdown = new QComboBox;
  FeedbackCategoryDropdown->setFixedWidth(WIDTH);
  FeedbackCategoryDropdown->setStyleSheet(FEEDBACK_CATEGORY_PANE_STYLING);
  FeedbackCategoryDropdown->addItems(categories);

  /* Feedback input */
  FeedbackTextPane = new QTextEdit;
  FeedbackTextPane->setFixedWidth(WIDTH);
  FeedbackTextPane->setStyleSheet(FEEDBACK_TEXT_PANE_STYLING);
  FeedbackTextPane->setReadOnly(false);

  /* Submit button */
  SubmitButton = new QPushButton("Send", this);
  SubmitButton->setStyleSheet(SUBMIT_BUTTON_STYLING);
  SubmitButton->setIcon(QPixmap( ":resources/send.png" ));
  SubmitButton->setMinimumWidth(110);
  SubmitButton->setMinimumHeight(40);

  // Connect button signal to appropriate slot
  connect(SubmitButton, SIGNAL(released()), this, SLOT(handleSubmitButton()));

  /* Main Content Layout */
  QGridLayout * mainContentLayout = new QGridLayout;
  mainContentLayout->addWidget(CloseButton, 0, 0, Qt::AlignRight);
  mainContentLayout->addWidget(feedbackLabel, 1, 0);
  mainContentLayout->addWidget(explanationLabel, 2, 0);
  mainContentLayout->addWidget(FeedbackCategoryDropdown, 3, 0);
  mainContentLayout->addWidget(FeedbackTextPane, 4, 0);
  mainContentLayout->addWidget(SubmitButton, 5, 0, Qt::AlignRight);

  mainContentLayout->setVerticalSpacing(10);

  QWidget * mainWidget = new QWidget(this);
  mainWidget->setLayout(mainContentLayout);

  setCentralWidget(mainWidget);

  setStyleSheet(APP_STYLING);

  createActions();

  setWindowTitle("Kano Feedback");

  // connect(runProcess, SIGNAL(readyReadStandardOutput()),
  //         this, SLOT(updateOutput()));

  connect(&app, SIGNAL( aboutToQuit() ), this, SLOT( onExitCleanup() ) );
}

/*******************************************************/
/*********************** Actions ***********************/
/*******************************************************/

void MainWindow::createActions()
{
  aboutAct = new QAction(tr("&About"), this);
  aboutAct->setStatusTip(tr("Show the application's About box"));
  connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

  aboutQtAct = new QAction(tr("About &Qt"), this);
  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
  connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

}

void MainWindow::handleSubmitButton()
{
  // Collect the data to send
  
  // Category
  int dropdownIndex = FeedbackCategoryDropdown->currentIndex();
  std::string category = categories.at(dropdownIndex).toLocal8Bit().constData();

  // Response
  std::string response = FeedbackTextPane->toPlainText().toStdString();

  // Kanux Version
  // Execute `ls -l /etc/kanux_version | awk '{ print $6 " " $7 " " $8 }'`
  char command[512];
  strcpy (command, "ls -l /etc/kanux_version | awk '{ print $6 \" \" $7 \" \" $8 }'");
  std::string kanux_version = executeCommand(command);

  // Running processes
  // Execute `ps -e | awk '{ print $4 }'`
  strcpy (command, "ps -e | awk '{ print $4 }'");
  std::string running_processes = executeCommand(command);

  // Running processes
  // Execute `dpkg-query -l | awk '{ print $2 "-" $3 }'`
  strcpy (command, "dpkg-query -l | awk '{ print $2 \"-\" $3 }'");
  std::string packages = executeCommand(command);

  // Dmesg
  // Execute `dmesg`
  strcpy (command, "dmesg");
  std::string dmesg = executeCommand(command);

  // Make sure they have entered a comment
  if (!response.compare(""))
    {
      QMessageBox noResponse;
      noResponse.setText(tr("You haven't entered a response"));
      noResponse.exec();
      return;
    }

  // Make sure they have selected a subject
  if (!category.compare("Subject") || !category.compare("--------------------------------------------------------"))
    {
      QMessageBox noSubject;
      noSubject.setText(tr("You haven't selected a subject"));
      noSubject.exec();
      return;
    }

  // If we don't have an e-mail address for the user, ask them to input it.
  // The e-mail is stored in ~/.useremail if one has been provided.
  std::string email_addr = "";

  std::ifstream file;
  std::string email_filename;
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;

  email_filename.append(homedir);
  email_filename.append("/.useremail");
  std::cout << "Opening file " << email_filename << "..." << "\n";
  file.open(email_filename.c_str());

  if (!file.is_open())
    {
      std::cout << "E-mail file not found\n";
    } else {
      std::cout << "E-mail file open\n";

      while (!file.eof())
        {
          getline(file, email_addr);
          // skip empty and CR+LF lines
          if (email_addr.length() > 3)
            {
              std::cout << "found e-mail: " << email_addr << "\n";
              break;
            }
        }

      file.close();
    }
  
  // Prompt for e-mail input. Default text is the found email.
  bool ok;
  QString text = QInputDialog::getText(this, tr("Your e-mail?"),
                                       tr("How can we contact you about your comments? Enter your e-mail"),
                                       QLineEdit::Normal, tr(email_addr.c_str() ), &ok);
  if (ok)
    {
      if (!emailValid(text.toUtf8().constData()))
        {
          QMessageBox blankEmail;
          blankEmail.setWindowTitle("Invalid e-mail provided");
          blankEmail.setText("You didn't enter a valid address.");
          blankEmail.setInformativeText("Try again.");
          blankEmail.setStandardButtons(QMessageBox::Ok);
          blankEmail.setDefaultButton(QMessageBox::Ok);
          blankEmail.exec();
          return;
        }
      std::cout << text.toStdString() << "\n";
      email_addr = text.toStdString();
    } else {
      // Return them to the window.
      return;
    }

  // Check that they want to send.
  QMessageBox confirmSendMsgBox;
  confirmSendMsgBox.setWindowTitle("Are you sure you want to send?");
  confirmSendMsgBox.setText("You are about to send us your improvements.");
  confirmSendMsgBox.setInformativeText("Do you want to send?");
  confirmSendMsgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  confirmSendMsgBox.setDefaultButton(QMessageBox::Ok);
  int confirmSendMsgBoxSelected = confirmSendMsgBox.exec();
  if (confirmSendMsgBoxSelected == QMessageBox::Cancel)
    {
      // Return them to the window.
      std::cout << "Will not send\n\n";
      return;
    }

  // We have everything. Form the object to send.
  std::string url = "https://docs.google.com/a/kano.me/forms/d/1PqWb05bQjjuHc41cA0m2f0jFgidUw_c5H53IQeaemgo/formResponse";

  std::string email_entry = "entry.1110323866";
  std::string category_entry = "entry.1341620943";
  std::string response_entry = "entry.162771870";
  std::string kanux_version_entry = "entry.1932769824";
  std::string running_processes_entry = "entry.868132968";
  std::string packages_entry = "entry.1747707726";
  std::string dmesg_entry = "entry.1892657243";

  std::string dataToSend;
  // E-mail
  dataToSend += "";
  dataToSend += email_entry;
  dataToSend += "=";
  dataToSend += removeQuotationMarks(email_addr);
  dataToSend += "&";
  // Category
  dataToSend += category_entry;
  dataToSend += "=";
  dataToSend += removeQuotationMarks(category);
  dataToSend += "&";
  // Response
  dataToSend += response_entry;
  dataToSend += "=";
  dataToSend += removeQuotationMarks(response);
  dataToSend += "&";
  // Kanux Version
  dataToSend += kanux_version_entry;
  dataToSend += "=";
  dataToSend += removeQuotationMarks(kanux_version);
  dataToSend += "&";
  // Running Processes
  dataToSend += running_processes_entry;
  dataToSend += "=";
  dataToSend += removeQuotationMarks(running_processes);
  dataToSend += "&";
  // Running Processes
  dataToSend += packages_entry;
  dataToSend += "=";
  dataToSend += removeQuotationMarks(packages);
  dataToSend += "&";
  // Dmesg
  dataToSend += dmesg_entry;
  dataToSend += "=";
  dataToSend += removeQuotationMarks(dmesg);


  // Send the data

  // Build the command-line command
  // curl --progress-bar -d 'entry.1110323866=email&entry.1341620943=category&entry.162771870=comment' https://docs.google.com/a/kano.me/forms/d/1PqWb05bQjjuHc41cA0m2f0jFgidUw_c5H53IQeaemgo/formResponse
  char uploadCommand[dataToSend.length() + 100];
  strcpy (uploadCommand, "curl --progress-bar -d \"");
  strcat (uploadCommand, dataToSend.c_str());
  strcat (uploadCommand, "\" https://docs.google.com/a/kano.me/forms/d/1PqWb05bQjjuHc41cA0m2f0jFgidUw_c5H53IQeaemgo/formResponse");
  // Execute the command
  std::string uploadResult = executeCommand(uploadCommand);

  // Parse the command output
  QMessageBox successBox;
  if (uploadResult.find("Thanks!") == std::string::npos)
  {
    // Upload failed
    std::cout << "Upload failed\n";
    std::cout << uploadResult << "\n";
    successBox.setWindowTitle("Failed!");
    successBox.setText(tr("I'm afraid that there was a problem uploading your thoughts. Check that you are connected to the internet and try again."));
    successBox.exec();
  } else {
    // Upload success
    std::cout << "Upload success\n";
    successBox.setWindowTitle("Success");
    successBox.setText(tr("Thank you for your help! We will use your views to improve Kano."));
    successBox.exec();
    close();
  }
  
}

/*******************************************
 **************** Utilities ****************
 *******************************************/

/***************** Executes the given command *****************
 * @param  {const char *} command  The command to be executed *
 * @return {std::string}   result  Output from command        *
 *                          -1     Error running command      *
 **************************************************************/
std::string MainWindow::executeCommand(const char* command)
{
  FILE* fp = NULL;

  std::cout << "Executing >>> " << command << "\n";

  // Execute the command
  fp = popen(command, "r");
  if (fp == NULL) {
      std::cout << "Error\n";
      return "-1";
  }

  // Store the stdout from the command so that it can be returned
  char buffer[128];
  std::string result = "";
  while(!feof(fp)) {
    if(fgets(buffer, 128, fp) != NULL)
      result += buffer;
  }

  // Clean up
  int status = pclose(fp);
  if (status == -1) {
    std::cout << "Error\n";
    return "-1";
  }

  return result;
}

/********************************** Replaces all " with ' **********************************
 **************** ensures that the data is in an appropriate format to send ****************
 * @param  {std::string} data  String to parse                                             *
 * @return {std::string} data  The parsed string                                           *
 *******************************************************************************************/
std::string MainWindow::removeQuotationMarks(std::string data)
{
  size_t found = data.find("\"");
  while (found != std::string::npos)
    {
      // Replace quotation mark
      data.replace(found, 1, "'");
      found = data.find("\"");
    }
  // Fix upload error when data field begins with " or '
  if (data.find("\"") == 0 || data.find("'") == 0 )
    {
      data = " " + data;
    }
  return data;
}

/************** Checks that a string is a valid e-mail address **************
 * @param  {std::string} email  E-mail to check                             *
 * @return {bool}         true  The e-mail is valid                         *
 *                       false  The e-mail is invalid                       *
 ****************************************************************************/
bool MainWindow::emailValid(std::string email)
{
  // Build the command-line command
  // echo email | egrep "[a-zA-Z0-9_\.-]+@[a-zA-Z0-9_.-]+\.[a-zA-Z0-9_.-]"
  char command[email.length() + 100];

  strcpy (command, "echo ");
  strcat (command, email.c_str());
  strcat (command, " | egrep \"[a-zA-Z0-9_\\.-]+@[a-zA-Z0-9_.-]+\\.[a-zA-Z0-9_.-]\"");
  
  // Execute the command
  std::string validResult = executeCommand(command);
  if (!validResult.compare(""))
    {
      return false;
    } else {
      return true;
    }
}

/****************************************************/
/*********************** Exit ***********************/
/****************************************************/

void MainWindow::onExit()
{
  close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{

}

void MainWindow::onExitCleanup()
{

}

/*******************************************************************/
/*********************** About/Help/Settings ***********************/
/*******************************************************************/

void MainWindow::about()
{
   QMessageBox::about(this, tr("About Kano Feedback"),
            tr("Use <b>Kano Feedback</b> to give us your thoughts on the Kano OS. "
               "Help us improve your experience!"));
}

/******************************************************************/
