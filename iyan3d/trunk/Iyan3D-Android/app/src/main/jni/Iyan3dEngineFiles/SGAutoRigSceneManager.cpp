//
//  SGAutoRigSceneManager.cpp
//  Iyan3D
//
//  Created by Karthik on 31/12/15.
//  Copyright © 2015 Smackall Games. All rights reserved.
//

#include "SGAutoRigSceneManager.h"
#include "SGEditorScene.h"

SGEditorScene* rigScene;

SGAutoRigSceneManager::SGAutoRigSceneManager(SceneManager* smgr, void *scene)
{
    this->smgr = smgr;
    rigScene = (SGEditorScene*)scene;
    sceneMode = RIG_MODE_OBJVIEW;
    nodeToRig = new SGNode(NODE_SGM);
    sgrSGNode = NULL;
    clearNodeSelections();
}

SGAutoRigSceneManager::~SGAutoRigSceneManager()
{    

}

void SGAutoRigSceneManager::clearNodeSelections()
{
    if(!rigScene || !smgr)
        return;
    
//    if(isNodeSelected){
//        rigScene->updater->updateControlsMaterial();
//    }
    isNodeSelected = false;
    isSkeletonJointSelected = false;
    isNodeSelected = false;
    isSGRJointSelected = false;
    selectedNodeId = NOT_SELECTED;
    selectedJointId = NOT_SELECTED;
    selectedNode = NULL;
    selectedJoint = NULL;
    scaleRatio = 0.0f;
}

void SGAutoRigSceneManager::sgmForRig(SGNode* sgNode)
{
    if(!rigScene || !smgr || !sgNode)
        return;

    if(nodeToRig->node){
        smgr->RemoveNode(nodeToRig->node);
    }
    nodeToRig = sgNode;
    sgmNode = dynamic_pointer_cast<MeshNode>(nodeToRig->node);

    // scale to fit all obj in same proportion-----
    float extendX = sgmNode->getMesh()->getBoundingBox()->getXExtend();
    float extendY = sgmNode->getMesh()->getBoundingBox()->getYExtend();
    float extendZ = sgmNode->getMesh()->getBoundingBox()->getZExtend();
    float max = ((extendX >= extendY) ? extendX:extendY);
    max = ((max >= extendZ) ? max:extendZ);
    scaleRatio = (max / OBJ_BOUNDINGBOX_MAX_LIMIT);
    sgmNode->setScale(Vector3(1.0/scaleRatio));
    //-----------
    
    sgmNode->setMaterial(smgr->getMaterialByIndex(SHADER_COMMON_L1));
    nodeToRig->props.isLighting = true;
    //objNode->setTexture(rigScene->shadowTexture,2);
    sgmNode->setID(SGM_ID);
    nodeToRig->node->updateAbsoluteTransformation();
}

bool SGAutoRigSceneManager::setSceneMode(AUTORIG_SCENE_MODE mode)
{
    if(!rigScene || !smgr)
        return false;

    switch(mode){
        case RIG_MODE_OBJVIEW:{
            if(sgrSGNode && sgrSGNode->node)
                Logger::log(ERROR,"SGAutoRigScene:setScenMode","SGrNod enot removed");
            
            if (rigKeys.size() > 0)
                removeRigKeys();
            if(nodeToRig){
                nodeToRig->node->setVisible(true);
                nodeToRig->node->setMaterial(smgr->getMaterialByIndex(SHADER_COMMON_L1));
                nodeToRig->props.transparency = 1.0;
                nodeToRig->props.isLighting = true;
            }
            break;
        }
            
        case RIG_MODE_MOVE_JOINTS:{
            if(nodeToRig){
                nodeToRig->node->setVisible(true);
                nodeToRig->node->setMaterial(smgr->getMaterialByIndex(SHADER_COMMON_L1));
                nodeToRig->props.transparency = 0.5;
            }
            rigScene->renHelper->setControlsVisibility(true);
            if(rigKeys.size()==0)
                initSkeletonJoints();
            else
                rigScene->renHelper->setJointAndBonesVisibility(rigKeys, true);
            
            rigScene->renHelper->setEnvelopVisibility(envelopes, false);
            
            if(sgrSGNode && sgrSGNode->node)
                sgrSGNode->node->setVisible(false);
            
            rigScene->controlType = MOVE;
            isNodeSelected = true;
            selectedNodeId = 0;
            selectedNode = rigKeys[0].referenceNode;
            rigScene->selectMan->updateSkeletonSelectionColors(0);
            printf("Skeleton Color Updated");
            rigScene->updater->updateControlsOrientaion();
            break;
            
        }
            
        case RIG_MODE_EDIT_ENVELOPES:{
            envelopes.clear();
            selectedNodeId = NOT_SELECTED;
            if(nodeToRig){
                nodeToRig->node->setVisible(true);
                nodeToRig->node->setMaterial(smgr->getMaterialByIndex(SHADER_VERTEX_COLOR_L1));
                nodeToRig->props.transparency = 0.5;
            }
            if(rigKeys.size() > 0 && rigKeys[0].referenceNode)
                rigScene->renHelper->setJointAndBonesVisibility(rigKeys, true);
            
            if(sgrSGNode && sgrSGNode->node) {
                rigScene->renHelper->setJointSpheresVisibility(false);
                
                sgrSGNode->clearSGJoints();
                smgr->RemoveNode(sgrSGNode->node);
            }
            sceneMode = RIG_MODE_EDIT_ENVELOPES;
            rigScene->controlType = MOVE;
            rigScene->renHelper->setControlsVisibility(false);
            rigScene->selectMan->updateSkeletonSelectionColors(0);
            rigScene->updater->updateOBJVertexColor();
            break;
        }
            
        case RIG_MODE_PREVIEW:{
            rigScene->renHelper->setControlsVisibility(true);
            rigScene->renHelper->setEnvelopVisibility(envelopes, false);
            if(nodeToRig){
                nodeToRig->node->setVisible(false);
            }
            rigScene->renHelper->setJointAndBonesVisibility(rigKeys, false);
            if(animatedSGRPath.compare(" ") != 0){
                if(sgrSGNode == NULL){
                    sgrSGNode = new SGNode(NODE_RIG);
                }
                if(sgrSGNode->node){
                    rigScene->renHelper->setJointSpheresVisibility(false);
                    sgrSGNode->clearSGJoints();
                    smgr->RemoveNode(sgrSGNode->node);
                }
                AnimatedMesh *mesh = CSGRMeshFileLoader::LoadMesh(animatedSGRPath, rigScene->shaderMGR->deviceType);
                sgrSGNode->setSkinningData((SkinMesh*)mesh);
                sgrSGNode->node = smgr->createAnimatedNodeFromMesh(mesh,"sgrUniforms",CHARACTER_RIG, MESH_TYPE_HEAVY);
                if(sgrSGNode->node){
                    sgrSGNode->createSGJoints();
                    sgrSGNode->node->setID(SGR_ID);
                    sgrSGNode->node->setMaterial(smgr->getMaterialByIndex(SHADER_COMMON_SKIN_L1));
                    sgrSGNode->node->setTexture(nodeToRig->node->getActiveTexture(),1);
                    sgrSGNode->node->setTexture(rigScene->shadowTexture,2);
                    sgrSGNode->props.transparency = 1.0;
                    sgrSGNode->props.isLighting = true;
                    sgrSGNode->node->setVisible(true);
                    printf("setscenemode - Vertices count %d ", dynamic_pointer_cast<MeshNode>(sgrSGNode->node)->getMesh()->getVerticesCount());
                }
            }
            else{
                Logger::log(ERROR,"SGAutoRigScene","SGR File Path Missing : " + std::string(animatedSGRPath));
            }
            rigScene->controlType = MOVE;
            break;
        }
            
        default:
            return false;
    }
    return true;
}

void SGAutoRigSceneManager::switchSceneMode(AUTORIG_SCENE_MODE mode){
    sceneMode = mode;
    clearNodeSelections();
    setSceneMode(mode);
}

void SGAutoRigSceneManager::removeRigKeys()
{
    for (int i = 0; i < rigScene->tPoseJoints.size(); i++) {
        if(rigKeys.find(rigScene->tPoseJoints[i].id) != rigKeys.end()){
            if(rigKeys[rigScene->tPoseJoints[i].id].bone){
                if(rigKeys[rigScene->tPoseJoints[i].id].bone->node)
                    smgr->RemoveNode(rigKeys[rigScene->tPoseJoints[i].id].bone->node);
                delete rigKeys[rigScene->tPoseJoints[i].id].bone;
                rigKeys[rigScene->tPoseJoints[i].id].bone = NULL;
            }
            if(rigKeys[rigScene->tPoseJoints[i].id].sphere){
                if(rigKeys[rigScene->tPoseJoints[i].id].sphere->node)
                    smgr->RemoveNode(rigKeys[rigScene->tPoseJoints[i].id].sphere->node);
                delete rigKeys[rigScene->tPoseJoints[i].id].sphere;
                rigKeys[rigScene->tPoseJoints[i].id].sphere = NULL;
            }
            if(rigKeys[rigScene->tPoseJoints[i].id].referenceNode){
                if(rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node)
                    smgr->RemoveNode(rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node);
                delete rigKeys[rigScene->tPoseJoints[i].id].referenceNode;
                rigKeys[rigScene->tPoseJoints[i].id].referenceNode = NULL;
            }
        }
    }
    rigKeys.clear();
    rigScene->tPoseJoints.clear();
}

void SGAutoRigSceneManager::initSkeletonJoints()
{
    rigKeys.clear();
    rigScene->tPoseJoints.clear();
    if(skeletonType == SKELETON_OWN)
        AutoRigJointsDataHelper::getTPoseJointsDataForOwnRig(rigScene->tPoseJoints,rigBoneCount);
    else
        AutoRigJointsDataHelper::getTPoseJointsData(rigScene->tPoseJoints);
    resetRigKeys();
}

bool SGAutoRigSceneManager::findInRigKeys(int key)
{
    return (rigKeys.find(key) != rigKeys.end());
}

void SGAutoRigSceneManager::resetRigKeys()
{
    for(int i = 0;i < rigScene->tPoseJoints.size();i++){
        if(rigKeys.find(rigScene->tPoseJoints[i].id) == rigKeys.end()){
            rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node = smgr->addEmptyNode();
            rigKeys[rigScene->tPoseJoints[i].id].parentId = rigScene->tPoseJoints[i].parentId;
            rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node->setPosition(Vector3(rigScene->tPoseJoints[i].position));
            rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node->setRotationInDegrees(Vector3(rigScene->tPoseJoints[i].rotation));
            rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node->updateAbsoluteTransformation();
            rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node->setID(REFERENCE_NODE_START_ID + i);
            rigKeys[rigScene->tPoseJoints[i].id].envelopeRadius = rigScene->tPoseJoints[i].envelopeRadius;
            if(rigScene->tPoseJoints[i].id != 0){
                sphereMesh = CSGRMeshFileLoader::createSGMMesh(constants::BundlePath + "/sphere.sgm", rigScene->shaderMGR->deviceType);
                rigKeys[rigScene->tPoseJoints[i].id].sphere->node = smgr->createNodeFromMesh(sphereMesh,"jointUniforms");
                rigKeys[rigScene->tPoseJoints[i].id].sphere->node->setID(JOINT_START_ID + i);
                rigKeys[rigScene->tPoseJoints[i].id].sphere->node->setParent(rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node);
                rigKeys[rigScene->tPoseJoints[i].id].sphere->node->setScale(Vector3(rigScene->tPoseJoints[i].sphereRadius));
                rigKeys[rigScene->tPoseJoints[i].id].sphere->node->updateAbsoluteTransformation();
                rigKeys[rigScene->tPoseJoints[i].id].sphere->node->setMaterial(smgr->getMaterialByIndex(SHADER_COLOR));
                rigKeys[rigScene->tPoseJoints[i].id].sphere->props.vertexColor = Vector3(SGR_JOINT_DEFAULT_COLOR_R,SGR_JOINT_DEFAULT_COLOR_G,SGR_JOINT_DEFAULT_COLOR_B);
            }
        }
    }
    
    //setting parent, child relationships for referenceNodes
    for(int i=0;i < rigScene->tPoseJoints.size();i++){
        int parentId = rigKeys[rigScene->tPoseJoints[i].id].parentId;
        if(parentId == -1)
            continue;
        rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node->setParent(rigKeys[parentId].referenceNode->node);
        rigKeys[rigScene->tPoseJoints[i].id].referenceNode->node->updateAbsoluteTransformationOfChildren();
    }
    //creating bones from each parentSphere
    for(int i=0;i < rigScene->tPoseJoints.size();i++){
        //if(rigKeys.find(tPoseJoints[i].id) != rigKeys.end()){
        if(!(rigKeys[rigScene->tPoseJoints[i].id].bone->node)){
            int parentId = rigKeys[rigScene->tPoseJoints[i].id].parentId;
            if(parentId <= 0) continue; //ignoring hip's parent
            boneMesh = CSGRMeshFileLoader::createSGMMesh(constants::BundlePath + "/BoneMesh.sgm", rigScene->shaderMGR->deviceType);
            rigKeys[rigScene->tPoseJoints[i].id].bone->node = smgr->createNodeFromMesh(boneMesh,"BoneUniforms");
            rigKeys[rigScene->tPoseJoints[i].id].bone->node->setMaterial(smgr->getMaterialByIndex(SHADER_COLOR));
            rigKeys[rigScene->tPoseJoints[i].id].bone->props.vertexColor = Vector3(1.0,1.0,1.0);
            rigKeys[rigScene->tPoseJoints[i].id].bone->props.transparency = 0.6;
            rigKeys[rigScene->tPoseJoints[i].id].bone->node->setID(BONE_START_ID + i);
            rigKeys[rigScene->tPoseJoints[i].id].bone->node->setParent(rigKeys[parentId].referenceNode->node);
            rigKeys[rigScene->tPoseJoints[i].id].bone->node->updateAbsoluteTransformation();
        }
    }
    for(int i=0;i < rigScene->tPoseJoints.size();i++)
        rigScene->updater->updateSkeletonBone(rigKeys, rigScene->tPoseJoints[i].id);
}

void SGAutoRigSceneManager::addNewJoint()
{
    if(!rigScene || !smgr || !rigScene->isRigMode)
        return;
    
    if(selectedNodeId == NOT_SELECTED || selectedNodeId == 0)
        return;
    
    if(rigKeys.size() >= RIG_MAX_BONES){
        boneLimitsCallBack();
        return;
    }
    rigScene->actionMan->addJointAction.drop();
    rigScene->actionMan->addJointAction.actionType = ACTION_ADD_JOINT;
    
    AutoRigJointsDataHelper::addNewTPoseJointData(rigScene->tPoseJoints,selectedNodeId);
    rigScene->actionMan->addJointAction.actionSpecificIntegers.push_back(selectedNodeId);
    
    rigScene->actionMan->addAction(rigScene->actionMan->addJointAction);
    resetRigKeys();
    
    int id = rigScene->tPoseJoints[rigScene->tPoseJoints.size()-1].id;
    if (rigKeys.find(id) != rigKeys.end()) {
        int prevSelectedBoneId = selectedNodeId;
        selectedNodeId = id;
        selectedNode = rigKeys[selectedNodeId].referenceNode;
        rigScene->selectMan->updateSkeletonSelectionColors(prevSelectedBoneId);
        rigScene->updater->updateControlsOrientaion();
    }
}

void SGAutoRigSceneManager::removeJoint()
{
    if(!rigScene || !smgr || !rigScene->isRigMode)
        return;

    int id = rigScene->tPoseJoints[rigScene->tPoseJoints.size()-1].id;
    rigKeyIterator = rigKeys.find(id);
    if(rigKeys[id].bone && rigKeys[id].bone->node){
        smgr->RemoveNode(rigKeys[id].bone->node);
        delete rigKeys[id].bone;
        rigKeys[id].bone = NULL;
    }
    if(rigKeys[id].sphere && rigKeys[id].sphere->node){
        smgr->RemoveNode(rigKeys[id].sphere->node);
        delete rigKeys[id].sphere;
        rigKeys[id].sphere = NULL;
    }
    if(rigKeys[id].referenceNode && rigKeys[id].referenceNode->node){
        smgr->RemoveNode(rigKeys[id].referenceNode->node);
        delete rigKeys[id].referenceNode;
        rigKeys[id].referenceNode = NULL;
    }
    
    rigKeys.erase(rigKeyIterator);
    rigScene->tPoseJoints.pop_back();
    resetRigKeys();
}

void SGAutoRigSceneManager::exportSGR(std::string filePath)
{
    if(!rigScene || !smgr || !rigScene->isRigMode)
        return;

    AutoRigHelper::initWeights(dynamic_pointer_cast<MeshNode>(nodeToRig->node),rigKeys,influencedVertices,influencedJoints);
    animatedSGRPath = filePath;
    exportSGR::createSGR(filePath,dynamic_pointer_cast<MeshNode>(nodeToRig->node),rigKeys,influencedVertices,influencedJoints);
}

void SGAutoRigSceneManager::setEnvelopeUniforms(int nodeID,string matName)
{
    if(!rigScene || !smgr)
        return;
    rigScene->shaderMGR->setUniforms(envelopes.find(nodeID - ENVELOPE_START_ID)->second,matName);
}
void SGAutoRigSceneManager::objNodeCallBack(string materialName)
{
    if(!rigScene || !smgr)
        return;
    rigScene->shaderMGR->setUniforms(nodeToRig,materialName);
}

void SGAutoRigSceneManager::boneNodeCallBack(int id,string materialName)
{
    if(!rigScene || !smgr)
        return;
    rigScene->shaderMGR->setUniforms(rigKeys[id - BONE_START_ID].bone,materialName);
}
void SGAutoRigSceneManager::jointNodeCallBack(int id,string materialName)
{
    if(!rigScene || !smgr)
        return;
    rigScene->shaderMGR->setUniforms(rigKeys[id - JOINT_START_ID].sphere,materialName);
}
void SGAutoRigSceneManager::setSGRUniforms(int jointId,string matName)
{
    if(!rigScene || !smgr)
        return;
    rigScene->shaderMGR->setUniforms(sgrSGNode,matName);
}
bool SGAutoRigSceneManager::isSGRTransparent(int jointId,string matName)
{
    if(!rigScene || !smgr)
        return false;
    return sgrSGNode->props.transparency < 1.0;
}

bool SGAutoRigSceneManager::isOBJTransparent(string materialName)
{
    if(!rigScene || !smgr)
        return false;
    return nodeToRig->props.transparency < 1.0;
}

SGNode* SGAutoRigSceneManager::getRiggedNode()
{
    return sgrSGNode;
}

void SGAutoRigSceneManager::changeEnvelopeScale(Vector3 scale, bool isChanged)
{
    if(selectedNodeId > 0)
    {
        rigKeys[selectedNodeId].envelopeRadius =  scale.x;
        if(rigScene->getMirrorState() == MIRROR_ON)
        {
            int mirrorJointId = BoneLimitsHelper::getMirrorJointId(selectedNodeId);
            if(mirrorJointId != -1){
                rigKeys[mirrorJointId].envelopeRadius = scale.x;
            }
        }
        updateEnvelopes();
        //updateOBJVertexColor();
        AutoRigHelper::updateOBJVertexColors(sgmNode, envelopes, rigKeys);
    }
}

void SGAutoRigSceneManager::updateEnvelopes()
{
    //update all visible envelopes.
    for(std::map<int, SGNode *>::iterator it = envelopes.begin(); it!=envelopes.end(); it++)
    {
        if(it->second && (selectedNodeId == rigKeys[it->first].parentId || it->first == selectedNodeId))
            initEnvelope(it->first);
        
        int mirrorJoint = BoneLimitsHelper::getMirrorJointId(selectedNodeId);
        
        if(rigScene->getMirrorState() == MIRROR_ON && it->second && (mirrorJoint == rigKeys[it->first].parentId || it->first == mirrorJoint))
            initEnvelope(it->first);
    }
}

void SGAutoRigSceneManager::initEnvelope(int jointId)
{
    SGNode *envelopeSgNod = (envelopes.find(jointId) == envelopes.end()) ? NULL:envelopes[jointId];
    if(jointId<=1) return;  //skipping envelope between hip and it's parent.
    int parentId = rigKeys[jointId].parentId;
    
    if(envelopeSgNod == NULL){
        envelopeSgNod = new SGNode(NODE_RIG);
        AnimatedMesh *mesh = CSGRMeshFileLoader::LoadMesh(constants::BundlePath + "/Envelop.sgr",rigScene->shaderMGR->deviceType);
        envelopeSgNod->setSkinningData((SkinMesh*)mesh);
        shared_ptr<AnimatedMeshNode> envelopeNode = smgr->createAnimatedNodeFromMesh(mesh,"envelopeUniforms",CHARACTER_RIG, MESH_TYPE_HEAVY);
        envelopeNode->setID(ENVELOPE_START_ID + jointId);
        envelopeNode->setParent(rigKeys[parentId].referenceNode->node);
        envelopeNode->setMaterial(smgr->getMaterialByIndex(SHADER_COLOR_SKIN));
        envelopeSgNod->node = envelopeNode;
        envelopeSgNod->props.vertexColor = Vector3(1.0);
        envelopeSgNod->props.transparency = 0.4;
    }
    if(envelopeSgNod->node) {
        envelopeSgNod->node->updateAbsoluteTransformation();
        
        Vector3 currentDir = BONE_BASE_DIRECTION;
        Vector3 targetDir = Vector3(rigKeys[jointId].referenceNode->node->getPosition()).normalize();
        envelopeSgNod->node->setRotationInDegrees(MathHelper::toEuler(MathHelper::rotationBetweenVectors(targetDir,currentDir))*RADTODEG);
        envelopeSgNod->node->updateAbsoluteTransformation();
        shared_ptr<JointNode> topJoint = (dynamic_pointer_cast<AnimatedMeshNode>(envelopeSgNod->node))->getJointNode(ENVELOPE_TOP_JOINT_ID);
        shared_ptr<JointNode> bottomJoint = (dynamic_pointer_cast<AnimatedMeshNode>(envelopeSgNod->node))->getJointNode(ENVELOPE_BOTTOM_JOINT_ID);
        
        float height = rigKeys[parentId].referenceNode->node->getAbsolutePosition().getDistanceFrom(rigKeys[jointId].referenceNode->node->getAbsolutePosition());
        topJoint->setPosition(Vector3(0,height,0));
        topJoint->updateAbsoluteTransformation();
        
        bottomJoint->setScale(Vector3(rigKeys[parentId].envelopeRadius));
        bottomJoint->updateAbsoluteTransformation();
        topJoint->setScale(Vector3(rigKeys[jointId].envelopeRadius));
        topJoint->updateAbsoluteTransformation();
        envelopes[jointId] = envelopeSgNod;
        envelopes[jointId]->node->setVisible(true);
        //topJoint.reset();
        //bottomJoint.reset();
    }
}

bool SGAutoRigSceneManager::deallocAutoRig(bool isCompleted)
{    
    if(sgrSGNode){
        if(sgrSGNode->node)
            smgr->RemoveNode(sgrSGNode->node);
        delete sgrSGNode;
    }
    if(!isCompleted){
        nodeToRig->props.transparency = 1.0;
        nodeToRig->node->setVisible(true);
    }
    if(isCompleted){
        for(int i = 0; i < rigScene->nodes.size(); i++){
            if(rigScene->nodes[i] == nodeToRig){
                rigScene->selectMan->selectObject(i);
                rigScene->actionMan->storeAddOrRemoveAssetAction(ACTION_ADD_BONE, 0);
                rigScene->loader->removeObject(i);
                break;
            }
        }
    }
    
    for(std::map<int, SGNode *>::iterator it = envelopes.begin(); it!=envelopes.end(); it++)
    {
        if (it->second) {
            delete it->second;
        }
    }
    envelopes.clear();
    removeRigKeys();
    sceneMode = RIG_MODE_OBJVIEW;
    clearNodeSelections();
    return true;
}
