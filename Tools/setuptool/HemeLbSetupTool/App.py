import wx

from HemeLbSetupTool.Model.Profile import Profile
from HemeLbSetupTool.Model.Pipeline import Pipeline

from HemeLbSetupTool.Controller.ProfileController import ProfileController
from HemeLbSetupTool.Controller.PipelineController import PipelineController

from HemeLbSetupTool.View.MainWindow import MainWindow

class SetupTool(wx.App):
    def __init__(self, args={}, profile=None, **kwargs):
        self.cmdLineArgs = args
        self.cmdLineProfileFile = profile
        
        wx.App.__init__(self, **kwargs)
        return
    
    def OnInit(self):
        # Model
        self.profile = Profile()
        
        self.pipeline = Pipeline()
        
        # Controller
        self.controller = ProfileController(self.profile)
        self.controller.Pipeline = PipelineController(self.pipeline, self.controller)
        
        # View
        self.view = MainWindow(self.controller)

        if self.cmdLineProfileFile is not None:
            # Load the profile
            self.profile.LoadFromFile(self.cmdLineProfileFile)
            pass
        
        # override any keys that have been set on cmdline.
        for k, val in self.cmdLineArgs.iteritems():
            if val is not None:
                setattr(self.profile, k, val)
                pass
            continue
        
        self.SetTopWindow(self.view)
        return True
    
    pass
