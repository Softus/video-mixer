function Controller() {
  installer.autoAcceptMessageBoxes();
  installer.installationFinished.connect(function() {
    gui.clickButton(buttons.NextButton);
  });
}

Controller.prototype.WelcomePageCallback = 
  Controller.prototype.CredentialsPageCallback = 
  Controller.prototype.ComponentSelectionPageCallback = 
  Controller.prototype.IntroductionPageCallback = 
  Controller.prototype.TargetDirectoryPageCallback =
  Controller.prototype.StartMenuDirectoryPageCallback =
  Controller.prototype.ReadyForInstallationPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
  gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
  gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function() {
var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm
  if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox) {
    checkBoxForm.launchQtCreatorCheckBox.checked = false;
  }
  gui.clickButton(buttons.FinishButton);
}
