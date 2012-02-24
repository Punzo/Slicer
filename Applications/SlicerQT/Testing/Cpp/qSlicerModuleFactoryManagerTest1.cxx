/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// QT includes

// SlicerQt includes
#include <qSlicerModuleFactoryManager.h>
#include <qSlicerCoreModuleFactory.h>
#include <qSlicerCoreApplication.h>

// STD includes


int qSlicerModuleFactoryManagerTest1(int argc, char * argv[])
{
  qSlicerCoreApplication app(argc, argv);
  Q_UNUSED(app);

  qSlicerModuleFactoryManager moduleFactoryManager;

  // Register factories
  moduleFactoryManager.registerFactory(new qSlicerCoreModuleFactory());

  // Register core modules
  moduleFactoryManager.registerModules();

  QString moduleName = "Cameras";

  moduleFactoryManager.instantiateModules();
  moduleFactoryManager.loadModules();

  qSlicerAbstractCoreModule * abstractModule =
    moduleFactoryManager.moduleInstance(moduleName);
  if( abstractModule == NULL )
    {
    moduleFactoryManager.printAdditionalInfo();
    std::cerr << __LINE__ << " - Error in loadModule()" << std::endl;
    return EXIT_FAILURE;
    }

  if( abstractModule->name() != moduleName )
    {
    moduleFactoryManager.printAdditionalInfo();
    std::cerr << __LINE__ << " - Error in moduleTitle() or moduleName()" << std::endl
              << "expected moduleName  = " << qPrintable( moduleName ) << std::endl
              << "real moduleName = " << qPrintable( abstractModule->name() ) << std::endl;
    return EXIT_FAILURE;
    }

  moduleFactoryManager.unloadModules();

  // Instantiate again
  moduleFactoryManager.instantiateModules();
  moduleFactoryManager.loadModules();
  abstractModule = moduleFactoryManager.moduleInstance(moduleName);

  if( abstractModule == NULL )
    {
    moduleFactoryManager.printAdditionalInfo();
    std::cerr << __LINE__ << " - Error in instantiateModule()" << std::endl;
    return EXIT_FAILURE;
    }

  moduleFactoryManager.unloadModules();

  return EXIT_SUCCESS;
}

