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

  This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
  and development was supported by the Center for Intelligent Image-guided Interventions (CI3).

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLThreeDSliceEdgeDisplayableManager.h"

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLColors.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkColor.h>
#include <vtkCutter.h>
#include <vtkFeatureEdges.h>
#include <vtkImageData.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkStripper.h>
#include <vtkTubeFilter.h>

// STD includes
#include <cmath>

//---------------------------------------------------------------------------
class EdgeRepresentation
{
public:
  EdgeRepresentation()
  {

  this->featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
  this->featureEdges->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);
  this->featureEdges->SetBoundaryEdges(true);
  this->featureEdges->SetFeatureEdges(false);
  this->featureEdges->SetNonManifoldEdges(false);
  this->featureEdges->SetManifoldEdges(false);
  this->featureEdges->SetColoring(false);

  this->stripper = vtkSmartPointer<vtkStripper>::New();
  this->stripper->SetInputConnection(featureEdges->GetOutputPort());

  this->tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
  this->tubeFilter->SetInputConnection(this->stripper->GetOutputPort());
  this->tubeFilter->SetRadius(0.5);
  this->tubeFilter->SetNumberOfSides(12);

  this->mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->mapper->SetInputConnection(tubeFilter->GetOutputPort());

  this->actor = vtkSmartPointer<vtkActor>::New();
  this->actor->SetMapper(this->mapper);

  vtkProperty* prop = this->actor->GetProperty();
  prop->SetRepresentationToSurface();
  prop->SetAmbient(0.5);
  prop->SetDiffuse(0.5);
  prop->SetSpecular(0);
  prop->SetShading(true);
  prop->SetSpecularPower(1);
  prop->SetOpacity(1.);
  prop->SetEdgeVisibility(false);
  prop->SetVertexVisibility(false);
  }
  ~EdgeRepresentation()
  {
  }

  vtkSmartPointer<vtkActor> actor;
  vtkSmartPointer<vtkPolyDataMapper> mapper;
  vtkSmartPointer<vtkFeatureEdges> featureEdges;
  vtkSmartPointer<vtkStripper> stripper;
  vtkSmartPointer<vtkTubeFilter> tubeFilter;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLThreeDSliceEdgeDisplayableManager);

//---------------------------------------------------------------------------
class vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal
{
public:
  typedef std::map<vtkMRMLSliceNode*, EdgeRepresentation*> SliceNodesLink;

  vtkInternal(vtkMRMLThreeDSliceEdgeDisplayableManager* external);
  ~vtkInternal();

  // View
  vtkMRMLViewNode* GetViewNode();

  // SliceNodes
  void AddSliceNode(vtkMRMLSliceNode*);
  void RemoveSliceNode(vtkMRMLSliceNode*);
  void RemoveSliceNode(SliceNodesLink::iterator);
  void RemoveAllSliceNodes();
  void UpdateSliceNodes();
  vtkMRMLSliceNode* GetSliceNode(EdgeRepresentation*);

  // Widget
  EdgeRepresentation* NewSliceEdgeRepresentation();
  EdgeRepresentation* GetEdgeRepresentation(vtkMRMLSliceNode*);
  // return with true if rendering is required
  bool UpdateEdgeRepresentation(vtkMRMLSliceNode*, EdgeRepresentation*);
  void ShowEdgeRepresentation(bool show, EdgeRepresentation* rep);
  bool createEdgeRepresentationMesh(EdgeRepresentation* rep,
                                    vtkMRMLSliceNode* sliceNode);

  SliceNodesLink                                SliceNodes;
  vtkMRMLThreeDSliceEdgeDisplayableManager*     External;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::vtkInternal(
  vtkMRMLThreeDSliceEdgeDisplayableManager* _external)
{
  this->External = _external;

}

//---------------------------------------------------------------------------
vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::~vtkInternal()
{
  this->RemoveAllSliceNodes();
}

//---------------------------------------------------------------------------
vtkMRMLViewNode* vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal
::GetViewNode()
{
  return this->External->GetMRMLViewNode();
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::AddSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode ||
     this->SliceNodes.find(vtkMRMLSliceNode::SafeDownCast(sliceNode)) !=
     this->SliceNodes.end())
  {
    return;
  }

  // We associate the node with the widget if an instantiation is called.
  // We add the sliceNode without instantiating the widget first.
  sliceNode->AddObserver(vtkCommand::ModifiedEvent,
                          this->External->GetMRMLNodesCallbackCommand());
  this->SliceNodes.insert(
    std::pair<vtkMRMLSliceNode*, EdgeRepresentation*>(sliceNode, static_cast<EdgeRepresentation*>(nullptr)));
  this->UpdateEdgeRepresentation(sliceNode, nullptr);
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::RemoveSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode)
  {
    return;
  }

  this->RemoveSliceNode(this->SliceNodes.find(vtkMRMLSliceNode::SafeDownCast(sliceNode)));
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::RemoveSliceNode(SliceNodesLink::iterator it)
{
  if (it == this->SliceNodes.end())
  {
    return;
  }

  // The manager has the responsabilty to delete the rep.
  if (it->second)
  {
    delete it->second;
  }

  // TODO: it->first might have already been deleted
  it->first->RemoveObserver(this->External->GetMRMLNodesCallbackCommand());
  this->SliceNodes.erase(it);
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::RemoveAllSliceNodes()
{
  // The manager has the responsabilty to delete the widgets.
  while (this->SliceNodes.size() > 0)
  {
    this->RemoveSliceNode(this->SliceNodes.begin());
  }
  this->SliceNodes.clear();
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::UpdateSliceNodes()
{
  if (this->External->GetMRMLScene() == nullptr)
  {
    this->RemoveAllSliceNodes();
    return;
  }

  vtkMRMLNode* node;
  vtkCollectionSimpleIterator it;
  vtkCollection* scene = this->External->GetMRMLScene()->GetNodes();
  for (scene->InitTraversal(it);
       (node = vtkMRMLNode::SafeDownCast(scene->GetNextItemAsObject(it))) ;)
  {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
    if (sliceNode)
    {
      this->AddSliceNode(sliceNode);
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::GetSliceNode(EdgeRepresentation* rep)
{
  if (!rep)
  {
    return nullptr;
  }

  // Get the slice node
  vtkMRMLSliceNode* sliceNode = nullptr;
  for (SliceNodesLink::iterator it=this->SliceNodes.begin();
       it!=this->SliceNodes.end(); ++it)
  {
    if (it->second == rep)
    {
      sliceNode = it->first;
      break;
    }
  }

  return sliceNode;
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::ShowEdgeRepresentation(bool show, EdgeRepresentation* rep)
{
  vtkRenderer* renderer = this->External->GetRenderer();
  if (!renderer)
  {
    return;
  }

  if (show)
  {
    renderer->AddActor(rep->actor);
  }
  else
  {
    renderer->RemoveActor(rep->actor);
  }
}

//---------------------------------------------------------------------------
EdgeRepresentation* vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::NewSliceEdgeRepresentation()
{
  EdgeRepresentation* rep = new EdgeRepresentation();
  return rep;
}

//---------------------------------------------------------------------------
EdgeRepresentation* vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::GetEdgeRepresentation(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode)
  {
    return nullptr;
  }

  SliceNodesLink::iterator it = this->SliceNodes.find(sliceNode);
  return (it != this->SliceNodes.end()) ? it->second : 0;
}

//---------------------------------------------------------------------------
bool vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::createEdgeRepresentationMesh(EdgeRepresentation* rep,
                                                                                         vtkMRMLSliceNode* sliceNode)
{
  if (!rep || !sliceNode)
  {
    return false;
  }

  vtkMRMLApplicationLogic *mrmlAppLogic = this->External->GetMRMLApplicationLogic();
  if (!mrmlAppLogic)
  {
    return false;
  }

  vtkMRMLSliceLogic* sliceLogic = mrmlAppLogic->GetSliceLogic(sliceNode);
  if (!sliceLogic)
  {
    return false;
  }

  vtkMRMLModelNode* sliceModelNode = sliceLogic->GetSliceModelNode();
  if (!sliceModelNode)
  {
    return false;
  }

  rep->featureEdges->SetInputConnection(sliceModelNode->GetPolyDataConnection());
  return true;
}

//---------------------------------------------------------------------------
bool vtkMRMLThreeDSliceEdgeDisplayableManager::vtkInternal::UpdateEdgeRepresentation(vtkMRMLSliceNode* sliceNode,
                                                                                     EdgeRepresentation* rep)
{
  if (!sliceNode || (!rep && !sliceNode->GetSliceVisible()))
  {
    return false;
  }

  if (!rep)
  {
    // Instantiate widget and link it if
    // there is no one associated to the sliceNode yet
    rep = this->NewSliceEdgeRepresentation();
    this->SliceNodes.find(sliceNode)->second = rep;
  }

  // Update rep
  if (!this->createEdgeRepresentationMesh(rep, sliceNode))
  {
    return false;
  }

  rep->actor->GetProperty()->SetColor(sliceNode->GetLayoutColor());

  bool visible =
    sliceNode->IsDisplayableInThreeDView(this->External->GetMRMLViewNode()->GetID())
    && sliceNode->GetSliceVisible();

  this->ShowEdgeRepresentation(visible, rep);

  return visible;
}

//---------------------------------------------------------------------------
// vtkMRMLSliceModelDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLThreeDSliceEdgeDisplayableManager::vtkMRMLThreeDSliceEdgeDisplayableManager()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLThreeDSliceEdgeDisplayableManager::~vtkMRMLThreeDSliceEdgeDisplayableManager()
{
  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::UnobserveMRMLScene()
{
  this->Internal->RemoveAllSliceNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::UpdateFromMRMLScene()
{
  this->Internal->UpdateSliceNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::OnMRMLSceneNodeAdded(vtkMRMLNode* nodeAdded)
{
  if (this->GetMRMLScene()->IsBatchProcessing() ||
      !nodeAdded->IsA("vtkMRMLSliceNode"))
  {
    return;
  }

  this->Internal->AddSliceNode(vtkMRMLSliceNode::SafeDownCast(nodeAdded));
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::OnMRMLSceneNodeRemoved(vtkMRMLNode* nodeRemoved)
{
  if (!nodeRemoved->IsA("vtkMRMLSliceNode"))
  {
    return;
  }

  this->Internal->RemoveSliceNode(vtkMRMLSliceNode::SafeDownCast(nodeRemoved));
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::OnMRMLNodeModified(vtkMRMLNode* node)
{
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
  assert(sliceNode);
  EdgeRepresentation* rep = this->Internal->GetEdgeRepresentation(sliceNode);
  if (this->Internal->UpdateEdgeRepresentation(sliceNode, rep))
  {
    this->RequestRender();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkMRMLThreeDSliceEdgeDisplayableManager::Create()
{
  this->Internal->UpdateSliceNodes();
}
